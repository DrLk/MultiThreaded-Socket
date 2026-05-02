#include "RemoteFileSystem.hpp"

#include <cstdint>
#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <string_view>
#include <unistd.h>

#include "FuseReadFileJob.hpp"
#include "FuseRequestTracker.hpp"
#include "Logger.hpp"
#include "RequestForgetMultiJob.hpp"
#include "RequestGetAttrJob.hpp"
#include "RequestLookupJob.hpp"
#include "RequestOpenDirJob.hpp"
#include "RequestOpenJob.hpp"
#include "RequestReadDirJob.hpp"
#include "RequestReadDirPlusJob.hpp"
#include "RequestReleaseDirJob.hpp"
#include "RequestReleaseJob.hpp"

#define TRACER() LOGGER() << "[RemoteFileSystem] " // NOLINT(cppcoreguidelines-macro-usage)

namespace {
FastTransport::FileSystem::FileHandle& GetFileHandle(fuse_file_info* fileInfo)
{
    return *(reinterpret_cast<FastTransport::FileSystem::FileHandle*>(fileInfo->fh)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}
} // namespace

namespace FastTransport::TaskQueue {
RemoteFileSystem::RemoteFileSystem(std::string_view mountPoint)
    : FileSystem(std::move(mountPoint))
{
    _getattr = &RemoteFileSystem::FuseGetattr;
    _lookup = &RemoteFileSystem::FuseLookup;
    _opendir = &RemoteFileSystem::FuseOpendir;
    _open = &RemoteFileSystem::FuseOpen;
    _forget = &RemoteFileSystem::FuseForget;
    _forgetMulti = &RemoteFileSystem::FuseForgetmulti;
    _release = &RemoteFileSystem::FuseRelease;
    _releaseDir = &RemoteFileSystem::FuseReleaseDir;
    _read = &RemoteFileSystem::FuseRead;
    _readDir = &RemoteFileSystem::FuseReadDir;
    _readDirPlus = &RemoteFileSystem::FuseReadDirPlus;
}

void RemoteFileSystem::FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(req, "getattr");
    TRACER() << "[getattr]"
             << " request: " << req
             << " inode: " << inode;

    const FastTransport::FileSystem::RemoteFileHandle* const remoteFile = fileInfo != nullptr ? GetFileHandle(fileInfo).remoteFile : nullptr;
    scheduler->Schedule(std::make_unique<RequestGetAttrJob>(req, inode, remoteFile));
}

void RemoteFileSystem::FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name)
{
    FUSE_TRACK(req, "lookup");
    TRACER() << "[lookup]"
             << " request: " << req
             << " parent: " << parentId
             << " name: " << name;

    scheduler->Schedule(std::make_unique<RequestLookupJob>(req, parentId, name));
}

void RemoteFileSystem::FuseOpendir(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "opendir");
    TRACER() << "[opendir]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestOpenDirJob>(request, inode, fileInfo));
}

void RemoteFileSystem::FuseOpen(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "open");
    TRACER() << "[open]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestOpenJob>(request, inode, fileInfo));
}

void RemoteFileSystem::FuseForget(fuse_req_t request, fuse_ino_t inode, std::uint64_t nlookup)
{
    FUSE_TRACK(request, "forget");
    TRACER() << "[forget]"
             << " request: " << request
             << " inode: " << inode
             << " nlookup: " << nlookup;

    fuse_forget_data forget = { .ino = inode, .nlookup = nlookup };
    scheduler->Schedule(std::make_unique<RequestForgetMultiJob>(request, std::span(&forget, 1)));
}

void RemoteFileSystem::FuseForgetmulti(fuse_req_t request, size_t count, fuse_forget_data* forgets)
{
    FUSE_TRACK(request, "forgetmulti");
    TRACER() << "[forgetmulti]"
             << " request: " << request
             << " count: " << count
             << " forgets: " << forgets;

    scheduler->Schedule(std::make_unique<RequestForgetMultiJob>(request, std::span(forgets, count)));
}

void RemoteFileSystem::FuseRelease(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "release");
    TRACER() << "[release]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestReleaseJob>(request, inode, &GetFileHandle(fileInfo)));
}

void RemoteFileSystem::FuseReleaseDir(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "releasedir");
    TRACER() << "[releasedir]"
             << " request: " << request
             << " inode: " << inode
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestReleaseDirJob>(request, inode, &GetFileHandle(fileInfo)));
}

void RemoteFileSystem::FuseRead(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "read");
    TRACER() << "[read]"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " off: " << offset
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<FileCache::FuseReadFileJob>(request, inode, size, offset, GetFileHandle(fileInfo).remoteFile));
}

void RemoteFileSystem::FuseReadDir(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "readdir");
    TRACER() << "[readdir]"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " off: " << offset
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestReadDirJob>(request, inode, size, offset, GetFileHandle(fileInfo).remoteFile));
}

void RemoteFileSystem::FuseReadDirPlus(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
{
    FUSE_TRACK(request, "readdirplus");
    TRACER() << "[readdir]"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " off: " << offset
             << " fileInfo: " << fileInfo;

    scheduler->Schedule(std::make_unique<RequestReadDirPlusJob>(request, inode, size, offset, GetFileHandle(fileInfo).remoteFile));
}

ITaskScheduler* RemoteFileSystem::scheduler = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
fuse_session* RemoteFileSystem::session = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

} // namespace FastTransport::TaskQueue
