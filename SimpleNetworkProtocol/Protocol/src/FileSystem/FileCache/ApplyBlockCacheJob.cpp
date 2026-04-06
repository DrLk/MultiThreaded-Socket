#include "ApplyBlockCacheJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <memory>

#include "FileCache/PinnedFuseBufVec.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "FuseRequestTracker.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "WriteFileCacheJob.hpp"

#define TRACER() LOGGER() << "[ApplyBlockCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ApplyBlockCacheJob::ApplyBlockCacheJob(fuse_req_t request, fuse_ino_t inode,
    size_t size, off_t offset, off_t skipped, Message&& data)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _skipped(skipped)
    , _data(std::move(data))
{
}

void ApplyBlockCacheJob::ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& tree)
{
    ZoneScopedN("ApplyBlockCacheJob::ExecuteCachedTree");

    const size_t readed = _data.empty()
        ? 0
        : ((_data.size() - 1) * _data.front()->GetPayload().size()) + _data.back()->GetPayload().size();
    const size_t replySize = std::min(readed + static_cast<size_t>(_skipped), _size);

    TRACER() << "inode=" << _inode
             << " offset=" << _offset
             << " skipped=" << _skipped
             << " readed=" << readed
             << " replySize=" << replySize;

    auto freePackets = tree.AddData(_inode, _offset + _skipped, readed, std::move(_data));

    if (_request != nullptr) {
        auto bufView = tree.GetData(_inode, _offset, replySize);
        auto buffVector = FileSystem::FileCache::buildPinnedBufVec(std::move(bufView));
        TRACER() << "reply replySize=" << replySize << " buffCount=" << buffVector->count;
        FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(_request), buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE));
    }

    const size_t blockIndex = static_cast<size_t>(_offset + _skipped) / static_cast<size_t>(FileSystem::Leaf::BlockSize);
    auto& leaf = GetLeaf(_inode, tree);
    for (auto& pendingJob : leaf.TakePendingJobs(blockIndex)) {
        pendingJob->Execute();
    }

    while (tree.NeedsEviction()) {
        auto [evictInode, evictOffset, evictSize, evictData] = tree.GetFreeData();
        if (evictData.empty()) {
            break;
        }
        scheduler.Schedule(std::make_unique<FileCache::WriteFileCacheJob>(
            evictInode, evictSize, evictOffset, std::move(evictData)));
    }

    if (!freePackets.empty()) {
        scheduler.ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
    }
}

} // namespace FastTransport::TaskQueue
