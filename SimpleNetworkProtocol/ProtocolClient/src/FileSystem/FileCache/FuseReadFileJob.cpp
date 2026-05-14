#include "FuseReadFileJob.hpp"
#include <Tracy.hpp>

#include <memory>

#include "FileCache/FileCache.hpp"
#include "FileCache/PinnedFuseBufVec.hpp"
#include "FuseRequestTracker.hpp"
#include "IClientTaskScheduler.hpp"
#include "IPendingJob.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"
#include "PieceStatus.hpp"
#include "PrefixDiskReadFileCacheJob.hpp"
#include "ReadFileCacheJob.hpp"
#include "RequestReadFileJob.hpp"

#define TRACER() LOGGER() << "[FuseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace {

// Pending job stored in Leaf when a required block is not yet in cache.
// Execute() is called by ResponseReadFileInJob when the block arrives, and reschedules
// FuseReadFileJob so it can re-evaluate the full cross-block read from scratch.
class FuseReadFilePendingJob : public FastTransport::FileSystem::IPendingJob {
public:
    FuseReadFilePendingJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset,
        FastTransport::FileSystem::RemoteFileHandle* remoteFile, size_t arrivedBlockIndex,
        FastTransport::FileSystem::Leaf* leaf, FastTransport::TaskQueue::ITaskScheduler* scheduler)
        : _request(request)
        , _inode(inode)
        , _size(size)
        , _offset(offset)
        , _remoteFile(remoteFile)
        , _arrivedBlockIndex(arrivedBlockIndex)
        , _leaf(leaf)
        , _scheduler(scheduler)
    {
    }

    void Execute() override
    {
        const auto arrivedBlockOffset = static_cast<off_t>(_arrivedBlockIndex * static_cast<size_t>(FastTransport::FileSystem::Leaf::BlockSize));
        const size_t requestBlockIndex = static_cast<size_t>(_offset) / static_cast<size_t>(FastTransport::FileSystem::Leaf::BlockSize);
        const auto requestBlockOffset = static_cast<off_t>(requestBlockIndex * static_cast<size_t>(FastTransport::FileSystem::Leaf::BlockSize));
        auto arrivedPin = std::move(_leaf->GetData(arrivedBlockOffset, static_cast<size_t>(FastTransport::FileSystem::Leaf::BlockSize)).pin);
        auto requestPin = std::move(_leaf->GetData(requestBlockOffset, static_cast<size_t>(FastTransport::FileSystem::Leaf::BlockSize)).pin);
        // Null _request before passing it so Cancel() won't double-reply.
        dynamic_cast<FastTransport::TaskQueue::IClientTaskScheduler&>(*_scheduler).ScheduleCacheTreeJob(std::make_unique<FastTransport::FileCache::FuseReadFileJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile, std::move(arrivedPin), std::move(requestPin)));
    }

    void Cancel() override
    {
        if (_request != nullptr) {
            LOGGER() << "FuseReadFilePendingJob cancelled before execution, replying EIO to unblock client";
            FUSE_REPLY_SHUTDOWN(fuse_reply_err(FUSE_UNTRACK(_request), EIO));
            _request = nullptr;
        }
    }

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    FastTransport::FileSystem::RemoteFileHandle* _remoteFile;
    size_t _arrivedBlockIndex;
    FastTransport::FileSystem::Leaf* _leaf;
    FastTransport::TaskQueue::ITaskScheduler* _scheduler;
};

} // anonymous namespace

namespace FastTransport::FileCache {

FuseReadFileJob::FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _remoteFile(remoteFile)
{
}

FuseReadFileJob::FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile,
    FileSystem::FileCache::RangePin arrivedBlockPin, FileSystem::FileCache::RangePin requestBlockPin)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _remoteFile(remoteFile)
    , _arrivedBlockPin(std::move(arrivedBlockPin))
    , _requestBlockPin(std::move(requestBlockPin))
{
}

FuseReadFileJob::~FuseReadFileJob() // NOLINT(bugprone-exception-escape)
{
    // Reply with EIO if this job is destroyed without executing (e.g. scheduler shut down
    // while the job was queued). Prevents the FUSE client from blocking indefinitely.
    // -ENOENT is expected: the requesting process may have died before we could reply.
    if (_request != nullptr) {
        LOGGER() << "FuseReadFileJob destroyed without executing, replying EIO to unblock client";
        FUSE_REPLY_SHUTDOWN(fuse_reply_err(FUSE_UNTRACK(_request), EIO));
    }
}

// Requests blockIndex from the network and registers this job as pending so it re-runs
// when the block arrives. Prefetches the next PrefetchAhead blocks to overlap RTTs.
// If the block is already InCache (arrived via prefetch before we registered), retries immediately.
// If the block is already InFlight (another request is fetching it), just registers as pending.
void FuseReadFileJob::FetchBlock(FileSystem::Leaf& leaf, size_t blockIndex, TaskQueue::ITaskScheduler& scheduler)
{
    ZoneScopedN("FuseReadFileJob::FetchBlock");
    auto makePendingJob = [&] {
        return std::make_unique<FuseReadFilePendingJob>(
            std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile, blockIndex, &leaf, &scheduler);
    };

    if (leaf.SetInFlight(blockIndex)) {
        leaf.AddPendingJob(blockIndex, makePendingJob());
        const auto blockOffset = static_cast<off_t>(blockIndex * static_cast<size_t>(Leaf::BlockSize));
        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(
            nullptr, _inode, static_cast<size_t>(Leaf::BlockSize), blockOffset, 0, _remoteFile));

        TriggerPrefetch(leaf, blockIndex, scheduler);
    } else if (leaf.GetPiecesStatus()->GetStatus(blockIndex) == FileSystem::PieceStatus::InCache) {
        TRACER() << "[loop-C] FetchBlock: blockIndex=" << blockIndex << " InCache (SetInFlight returned false)"
                 << " _offset=" << _offset << " _size=" << _size;
        dynamic_cast<TaskQueue::IClientTaskScheduler&>(scheduler).ScheduleCacheTreeJob(std::make_unique<FuseReadFileJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile));
    } else {
        leaf.AddPendingJob(blockIndex, makePendingJob());
    }
}

void FuseReadFileJob::TriggerPrefetch(FileSystem::Leaf& leaf, size_t blockIndex, TaskQueue::ITaskScheduler& scheduler)
{
    ZoneScopedN("FuseReadFileJob::TriggerPrefetch");
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
}

void FuseReadFileJob::ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree)
{
    ZoneScopedN("FuseReadFileJob::ExecuteCachedTree");
    if (stop.stop_requested()) {
        TRACER() << "Job cancelled";
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(std::exchange(_request, nullptr)), EIO));
        return;
    }
    TRACER() << "[read]"
             << " request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " offset: " << _offset
             << " remoteFile: " << _remoteFile;

    auto& leaf = _inode == FUSE_ROOT_ID ? tree.GetRoot() : *(reinterpret_cast<Leaf*>(_inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

    if (leaf.GetSize() <= _offset) {
        TRACER() << "File size is less than offset";
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(std::exchange(_request, nullptr)), EIO));
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

            const size_t blockEnd = (blockIndex + 1) * static_cast<size_t>(Leaf::BlockSize);
            if (static_cast<size_t>(_offset) + _size > blockEnd) {
                const size_t nextBlockIndex = blockEnd / static_cast<size_t>(Leaf::BlockSize);
                const auto nextStatus = leaf.GetPiecesStatus()->GetStatus(nextBlockIndex);

                if (nextStatus != FileSystem::PieceStatus::OnDisk) {
                    // B+1 is not on disk — try to get it from memory.
                    const size_t secondPartSize = static_cast<size_t>(_offset) + _size - blockEnd;
                    auto pinned = leaf.GetData(static_cast<off_t>(blockEnd), secondPartSize);
                    if (pinned.HasData()) {
                        // B+1 in memory: disk B (first part) + memory B+1 (second part).
                        scheduler.Schedule(std::make_unique<ReadFileCacheJob>(std::exchange(_request, nullptr), file, _size, _offset,
                            buildPinnedBufVec(std::move(pinned))));
                        return;
                    }

                    // B+1 not in memory yet — wait for it rather than issuing a short disk read.
                    if (nextStatus == FileSystem::PieceStatus::InCache) {
                        TRACER() << "[loop-A] B=" << blockIndex << " OnDisk, B+1=" << nextBlockIndex
                                 << " InCache but GetData(blockEnd=" << blockEnd << ", secondPartSize=" << secondPartSize << ") HasData=false"
                                 << " _offset=" << _offset << " _size=" << _size;
                        dynamic_cast<TaskQueue::IClientTaskScheduler&>(scheduler).ScheduleCacheTreeJob(std::make_unique<FuseReadFileJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile));
                        return;
                    }
                    if (leaf.SetInFlight(nextBlockIndex)) {
                        leaf.AddPendingJob(nextBlockIndex, std::make_unique<FuseReadFilePendingJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile, nextBlockIndex, &leaf, &scheduler));
                        const auto nextBlockOffset = static_cast<off_t>(nextBlockIndex * static_cast<size_t>(Leaf::BlockSize));
                        scheduler.Schedule(std::make_unique<TaskQueue::RequestReadFileJob>(
                            nullptr, _inode, static_cast<size_t>(Leaf::BlockSize), nextBlockOffset, 0, _remoteFile));
                    } else if (leaf.GetPiecesStatus()->GetStatus(nextBlockIndex) == FileSystem::PieceStatus::InCache) {
                        TRACER() << "[loop-B] B=" << blockIndex << " OnDisk, B+1=" << nextBlockIndex
                                 << " became InCache between SetInFlight check, _offset=" << _offset;
                        dynamic_cast<TaskQueue::IClientTaskScheduler&>(scheduler).ScheduleCacheTreeJob(std::make_unique<FuseReadFileJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile));
                        return;
                    } else {
                        leaf.AddPendingJob(nextBlockIndex, std::make_unique<FuseReadFilePendingJob>(std::exchange(_request, nullptr), _inode, _size, _offset, _remoteFile, nextBlockIndex, &leaf, &scheduler));
                    }
                    return;
                }
            }

            // Either no cross-block or both B and B+1 are on disk: full disk read is correct.
            scheduler.Schedule(std::make_unique<ReadFileCacheJob>(std::exchange(_request, nullptr), file, _size, _offset));
            return;
        }

        // Use pending mechanism so FuseReadFileJob re-runs when the block arrives. Embedding
        // _request directly in RequestReadFileJob would produce a short reply for cross-block
        // reads: replySize covers only bytes within block B, missing the B+1 tail.
        FetchBlock(leaf, blockIndex, scheduler);
        return;
    }

    const size_t totalBytes = buffer.TotalSize();

    if (totalBytes < _size) {
        // Partial data: FUSE read spans two cache blocks and the next block is not yet in cache.
        TRACER() << "[partial] totalBytes=" << totalBytes << " _size=" << _size << " _offset=" << _offset;
        const auto skipped = static_cast<off_t>(totalBytes);
        const size_t nextBlockIndex = static_cast<size_t>(_offset + skipped) / static_cast<size_t>(Leaf::BlockSize);
        if (leaf.GetPiecesStatus()->GetStatus(nextBlockIndex) == FileSystem::PieceStatus::OnDisk) {
            // Block B is InCache (buffer holds it) and B+1 is OnDisk.
            // Do NOT read from disk at _offset: block B is not on disk, so the disk cache at
            // _offset is stale/empty. Use buffer as a prefix and read only the B+1 portion from disk.
            const FileSystem::NativeFile::Ptr file = tree.GetFileCache().GetFile(tree.GetCacheFolder() / leaf.GetCachePath());
            const size_t secondPartSize = _size - totalBytes;
            const auto diskOffset = static_cast<off_t>(nextBlockIndex * static_cast<size_t>(Leaf::BlockSize));
            scheduler.Schedule(std::make_unique<PrefixDiskReadFileCacheJob>(std::exchange(_request, nullptr), buildPinnedBufVec(std::move(buffer)), file, diskOffset, secondPartSize));
            return;
        }

        // Block B may be evicted during the network round-trip for B+1. Re-running via pending
        // re-evaluates block B's state (InCache / OnDisk / NotFound) at reply time.
        FetchBlock(leaf, nextBlockIndex, scheduler);
        return;
    }

    const size_t blockIndex = static_cast<size_t>(_offset) / static_cast<size_t>(Leaf::BlockSize);
    TriggerPrefetch(leaf, blockIndex, scheduler);

    auto pinned = buildPinnedBufVec(std::move(buffer));
    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(std::exchange(_request, nullptr)), pinned.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE));
}

} // namespace FastTransport::FileCache
