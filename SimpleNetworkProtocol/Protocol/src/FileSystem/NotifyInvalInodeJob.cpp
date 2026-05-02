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

    const FileSystem::Leaf* const leaf = fileTree.FindLeafByServerInode(serverInode);
    if (leaf != nullptr) {
        const auto clientInode = reinterpret_cast<fuse_ino_t>(leaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
        scheduler.ScheduleCacheTreeJob(std::make_unique<InvalidateCacheLeafJob>(
            clientInode, static_cast<fuse_ino_t>(serverInode), RemoteFileSystem::session));
    }

    return GetFreeReadPackets();
}

} // namespace FastTransport::TaskQueue
