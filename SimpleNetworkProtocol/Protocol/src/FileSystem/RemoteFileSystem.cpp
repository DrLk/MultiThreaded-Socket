#include "RemoteFileSystem.hpp"

#include <memory>
#include <string_view>

#include "Logger.hpp"
#include "RequestGetAttrJob.hpp"
#include "RequestLookupJob.hpp"
#include "RequestOpenDirJob.hpp"

#define TRACER() LOGGER() << "[RemoteFileSystem] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {
RemoteFileSystem::RemoteFileSystem(std::string_view mountPoint)
    : FileSystem(std::move(mountPoint))
{
    _getattr = &RemoteFileSystem::FuseGetattr;
    _lookup = &RemoteFileSystem::FuseLookup;
    _opendir = &RemoteFileSystem::FuseOpendir;
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

void RemoteFileSystem::FuseOpendir(fuse_req_t request, fuse_ino_t inode, struct fuse_file_info* fileInfo)
{
    TRACER() << "[opendir]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestOpenDirJob>(request, inode, fileInfo));
}

ITaskScheduler* RemoteFileSystem::scheduler = nullptr;

} // namespace FastTransport::TaskQueue
