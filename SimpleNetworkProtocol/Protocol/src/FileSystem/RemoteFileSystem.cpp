#include "RemoteFileSystem.hpp"

#include <memory>
#include <string_view>

#include "Logger.hpp"
#include "RequestGetAttrJob.hpp"
#include "RequestLookupJob.hpp"

#define TRACER() LOGGER() << "[RemoteFileSystem] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {
RemoteFileSystem::RemoteFileSystem(std::string_view mountPoint)
    : FileSystem(std::move(mountPoint))
{
    _getattr = &RemoteFileSystem::FuseGetattr;
    _lookup = &RemoteFileSystem::FuseLookup;
}

void RemoteFileSystem::FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[getattr]"
             << " request: " << req
             << " inode: " << inode;

    scheduler->Schedule(std::make_unique<RequestGetAttrJob>(req, inode, fileInfo));
}

void RemoteFileSystem::FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name)
{
    TRACER() << "[lookup]"
             << " request: " << req
             << " parent: " << parentId
             << " name: " << name;

    scheduler->Schedule(std::make_unique<RequestLookupJob>(req, parentId, name));
}

ITaskScheduler* RemoteFileSystem::scheduler = nullptr;

} // namespace FastTransport::TaskQueue
