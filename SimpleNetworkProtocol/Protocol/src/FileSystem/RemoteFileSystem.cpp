#include "RemoteFileSystem.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <string_view>

#include "Logger.hpp"
#include "RequestForgetMultiJob.hpp"
#include "RequestGetAttrJob.hpp"
#include "RequestLookupJob.hpp"
#include "RequestOpenDirJob.hpp"
#include "RequestReleaseJob.hpp"

#define TRACER() LOGGER() << "[RemoteFileSystem] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {
RemoteFileSystem::RemoteFileSystem(std::string_view mountPoint)
    : FileSystem(std::move(mountPoint))
{
    _getattr = &RemoteFileSystem::FuseGetattr;
    _lookup = &RemoteFileSystem::FuseLookup;
    _opendir = &RemoteFileSystem::FuseOpendir;
    _forget = &RemoteFileSystem::FuseForget;
    _forgetMulti = &RemoteFileSystem::FuseForgetmulti;
    _release = &RemoteFileSystem::FuseRelease;
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

void RemoteFileSystem::FuseOpendir(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[opendir]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestOpenDirJob>(request, inode, fileInfo));
}

void RemoteFileSystem::FuseForget(fuse_req_t request, fuse_ino_t inode, std::uint64_t nlookup)
{
    TRACER() << "[forget]"
             << " request: " << request
             << " inode: " << inode
             << " nlookup: " << nlookup;

    /*auto* fileInfo = new fuse_file_info();
    scheduler->Schedule(std::make_unique<RequestGetAttrJob>(request, inode, fileInfo));
    return;*/
    fuse_forget_data forget = {inode, nlookup};
    scheduler->Schedule(std::make_unique<RequestForgetMultiJob>(request, std::span(&forget, 1)));
}

void RemoteFileSystem::FuseForgetmulti(fuse_req_t request, size_t count, fuse_forget_data* forgets)
{
    TRACER() << "[forgetmulti]"
             << " request: " << request
             << " count: " << count
             << " forgets: " << forgets;

    scheduler->Schedule(std::make_unique<RequestForgetMultiJob>(request, std::span(forgets, count)));
}

void RemoteFileSystem::FuseRelease(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    TRACER() << "[release]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestReleaseJob>(request, inode, fileInfo));
}

ITaskScheduler* RemoteFileSystem::scheduler = nullptr;

} // namespace FastTransport::TaskQueue
