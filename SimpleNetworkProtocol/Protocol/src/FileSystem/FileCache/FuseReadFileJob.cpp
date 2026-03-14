#include "FuseReadFileJob.hpp"

#include <memory>
#include <span>
#include <vector>

#include "FileCache/FileCache.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"
#include "PieceStatus.hpp"
#include "ReadFileCacheJob.hpp"
#include "RequestReadFileJob.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

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

    std::unique_ptr<fuse_bufvec> buffer = tree.GetData(_inode, _offset, _size);

    if (!buffer || buffer->count == 0) {
        const size_t blockIndex = static_cast<size_t>(_offset) / static_cast<size_t>(Leaf::BlockSize);

        if (leaf.GetPiecesStatus()->GetStatus(blockIndex) == FileSystem::PieceStatus::OnDisk) {
            const FileSystem::NativeFile::Ptr file = tree.GetFileCache().GetFile(tree.GetCacheFolder() / leaf.GetCachePath());

            // If the read spans into the next block and that block is NOT on disk (it's in
            // memory), the cache file ends at the block boundary and preadv would return a
            // short read.  Pre-copy the second block's portion from memory so
            // ReadFileCacheJob can assemble a full reply.
            std::vector<char> appendData;
            const size_t blockEnd = (blockIndex + 1) * static_cast<size_t>(Leaf::BlockSize);
            if (static_cast<size_t>(_offset) + _size > blockEnd) {
                const size_t nextBlockIndex = blockEnd / static_cast<size_t>(Leaf::BlockSize);
                if (leaf.GetPiecesStatus()->GetStatus(nextBlockIndex) != FileSystem::PieceStatus::OnDisk) {
                    const size_t secondPartSize = static_cast<size_t>(_offset) + _size - blockEnd;
                    auto secondBuffer = tree.GetData(_inode, static_cast<off_t>(blockEnd), secondPartSize);
                    if (secondBuffer && secondBuffer->count > 0) {
                        for (const auto& bufEntry : std::span<fuse_buf>(std::data(secondBuffer->buf), secondBuffer->count)) {
                            const auto* mem = static_cast<const char*>(bufEntry.mem);
                            appendData.insert(appendData.end(), mem, std::next(mem, static_cast<std::ptrdiff_t>(bufEntry.size)));
                        }
                    }
                }
            }

            scheduler.Schedule(std::make_unique<ReadFileCacheJob>(_request, file, _size, _offset, std::move(appendData)));
            return;
        }

        const off_t skipped = -static_cast<off_t>(_offset % Leaf::BlockSize);
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, skipped, _remoteFile));
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
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(_request, _inode, _size, _offset, skipped, _remoteFile));
        return;
    }

    fuse_reply_data(_request, buffer.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
}

} // namespace FastTransport::FileCache
