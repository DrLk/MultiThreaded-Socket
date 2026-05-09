#include "NotifyInvalInodeJob.hpp"

#include <cstdint>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileSystem/FileCache/InvalidateCacheLeafJob.hpp"
#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"
#include "RemoteFileSystem.hpp"

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message NotifyInvalInodeJob::ExecuteResponse(ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& fileTree)
{
    auto& reader = GetReader();
    std::uint64_t serverInode = 0;
    reader >> serverInode;

    auto leaf = fileTree.FindLeafByServerInode(serverInode);
    if (leaf != nullptr) {
        // The job holds the leaf shared_ptr itself — that keeps the Leaf
        // alive across the dispatch hop to the cacheTreeQueue shard without
        // needing to touch nlookup/_selfPin (which would force them into
        // multi-thread access).
        const auto clientInode = GetINode(*leaf);
        scheduler.ScheduleCacheTreeJob(std::make_unique<InvalidateCacheLeafJob>(
            std::move(leaf), clientInode, RemoteFileSystem::session));
    }

    return GetFreeReadPackets();
}

} // namespace FastTransport::TaskQueue
