#include "ResponseReadFileInJob.hpp"

#include <algorithm>
#include <cstddef>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "FileCache/FuseReadFileJob.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"
#include "WriteFileCacheJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadFileInJob::ExecuteResponse(ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& fileTree)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    off_t skipped = 0;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> skipped;
    reader >> error;

    TRACER() << "Execute"
             << " request=" << request
             << " inode=" << inode
             << " size=" << size
             << " offset=" << offset
             << " skipped=" << skipped
             << " error=" << error;

    if (error != 0) {
        fuse_reply_err(request, error);
        return {};
    }

    Message data;
    reader >> data;

    const size_t readed = data.empty() ? 0 : ((data.size() - 1) * data.front()->GetPayload().size()) + data.back()->GetPayload().size();
    const size_t replySize = std::min(readed + skipped, size);

    auto freePackets = fileTree.AddData(inode, offset + skipped, readed, std::move(data));
    if (request != nullptr) {
        auto buffVector = fileTree.GetData(inode, offset, replySize);
        TRACER() << "reply replySize=" << replySize << " buffCount=" << buffVector->count
                 << " offset=" << offset << " skipped=" << skipped << " readed=" << readed;
        fuse_reply_data(request, buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
    }

    const size_t blockIndex = static_cast<size_t>(offset + skipped) / static_cast<size_t>(FileSystem::Leaf::BlockSize);
    auto& leaf = inode == FUSE_ROOT_ID // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
        ? fileTree.GetRoot()
        : *(reinterpret_cast<FileSystem::Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    for (auto& pending : leaf.TakePendingRequests(blockIndex)) {
        scheduler.ScheduleCacheTreeJob(std::make_unique<FileCache::FuseReadFileJob>(
            pending.request, pending.inode, pending.size, pending.offset, pending.remoteFile));
    }

    while (fileTree.NeedsEviction()) {
        auto [evictInode, evictOffset, evictSize, evictData] = fileTree.GetFreeData();
        if (evictData.empty()) {
            break;
        }
        scheduler.Schedule(std::make_unique<FileCache::WriteFileCacheJob>(evictInode, evictSize, evictOffset, std::move(evictData)));
    }
    return freePackets;
}

} // namespace FastTransport::TaskQueue
