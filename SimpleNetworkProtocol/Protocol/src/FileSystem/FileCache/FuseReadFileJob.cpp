#include "FuseReadFileJob.hpp"

#include <memory>

#include "FileCache/FileCache.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"
#include "PieceStatus.hpp"
#include "ReadFileCacheJob.hpp"
#include "RequestReadFileJob.hpp"

#define TRACER() LOGGER() << "[FuseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

FuseReadFileJob::FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _remoteFile(remoteFile)
{
}

void FuseReadFileJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& tree)
{
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " offset: " << _offset
             << " remoteFile: " << _remoteFile;

    auto& leaf = _inode == FUSE_ROOT_ID ? tree.GetRoot() : *(reinterpret_cast<Leaf*>(_inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    if (leaf.GetSize() <= _offset) {
        TRACER() << "File size is less than offset";
        fuse_reply_err(_request, EIO);
        return;
    }

    if (leaf.GetSize() < _offset + _size) {
        TRACER() << "Last block is not full";
        _size = leaf.GetSize() - _offset;
    }

    auto buffer = tree.GetData(_inode, _offset, _size);

    if (!buffer.HasData()) {
        const size_t blockIndex = static_cast<size_t>(_offset) / static_cast<size_t>(Leaf::BlockSize);

        if (leaf.GetPiecesStatus()->GetStatus(blockIndex) == FileSystem::PieceStatus::OnDisk) {
            const FileSystem::NativeFile::Ptr file = tree.GetFileCache().GetFile(tree.GetCacheFolder() / leaf.GetCachePath());

            // If the read spans into the next block and that block is NOT on disk (it's in
            // memory), pass a PinnedFuseBufVec pointing directly into the Leaf's packet
            // payloads — no copy. The embedded RangePin keeps those Ranges alive until the
            // job is done, after which they remain in the Leaf cache for later disk eviction.
            FileSystem::FileCache::PinnedFuseBufVec appendData;
            const size_t blockEnd = (blockIndex + 1) * static_cast<size_t>(Leaf::BlockSize);
            if (static_cast<size_t>(_offset) + _size > blockEnd) {
                const size_t nextBlockIndex = blockEnd / static_cast<size_t>(Leaf::BlockSize);
                if (leaf.GetPiecesStatus()->GetStatus(nextBlockIndex) != FileSystem::PieceStatus::OnDisk) {
                    const size_t secondPartSize = static_cast<size_t>(_offset) + _size - blockEnd;
                    auto pinned = leaf.GetData(static_cast<off_t>(blockEnd), secondPartSize);
                    if (pinned.HasData()) {
                        appendData = std::move(pinned);
                    }
                }
            }

            scheduler.Schedule(std::make_unique<ReadFileCacheJob>(_request, file, _size, _offset,
                std::move(appendData)));
            return;
        }

        if (leaf.SetInFlight(blockIndex)) {
            const off_t skipped = -static_cast<off_t>(_offset % Leaf::BlockSize);
            scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, skipped, _remoteFile));

            // Prefetch upcoming blocks to overlap network RTTs
            constexpr size_t PrefetchAhead = 4;
            for (size_t i = 1; i <= PrefetchAhead; i++) {
                const size_t prefetchBlockIndex = blockIndex + i;
                const auto prefetchOffset = static_cast<off_t>(prefetchBlockIndex * static_cast<size_t>(Leaf::BlockSize));
                if (static_cast<size_t>(prefetchOffset) >= leaf.GetSize()) {
                    break;
                }
                if (leaf.SetInFlight(prefetchBlockIndex)) {
                    scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(
                        nullptr, _inode, static_cast<size_t>(Leaf::BlockSize), prefetchOffset, 0, _remoteFile));
                }
            }
        } else {
            leaf.AddPendingRequest(blockIndex, { .request = _request, .inode = _inode, .size = _size, .offset = _offset, .remoteFile = _remoteFile });
        }
        return;
    }

    size_t totalBytes = 0;
    for (size_t i = 0; i < buffer->count; ++i) {
        totalBytes += buffer->buf[i].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    if (totalBytes < _size) {
        // Partial data: FUSE read spans two cache blocks and the next block is not in cache.
        const auto skipped = static_cast<off_t>(totalBytes);
        const size_t nextBlockIndex = static_cast<size_t>(_offset + skipped) / static_cast<size_t>(Leaf::BlockSize);
        if (leaf.GetPiecesStatus()->GetStatus(nextBlockIndex) == FileSystem::PieceStatus::OnDisk) {
            const FileSystem::NativeFile::Ptr file = tree.GetFileCache().GetFile(tree.GetCacheFolder() / leaf.GetCachePath());
            scheduler.Schedule(std::make_unique<ReadFileCacheJob>(_request, file, _size, _offset));
            return;
        }
        if (leaf.SetInFlight(nextBlockIndex)) {
            scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, skipped, _remoteFile));

            // Prefetch upcoming blocks to overlap network RTTs
            constexpr size_t PrefetchAhead = 4;
            for (size_t i = 1; i <= PrefetchAhead; i++) {
                const size_t prefetchBlockIndex = nextBlockIndex + i;
                const auto prefetchOffset = static_cast<off_t>(prefetchBlockIndex * static_cast<size_t>(Leaf::BlockSize));
                if (static_cast<size_t>(prefetchOffset) >= leaf.GetSize()) {
                    break;
                }
                if (leaf.SetInFlight(prefetchBlockIndex)) {
                    scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(
                        nullptr, _inode, static_cast<size_t>(Leaf::BlockSize), prefetchOffset, 0, _remoteFile));
                }
            }
        } else {
            leaf.AddPendingRequest(nextBlockIndex, { .request = _request, .inode = _inode, .size = _size, .offset = _offset, .remoteFile = _remoteFile });
        }
        return;
    }

    fuse_reply_data(_request, buffer.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
