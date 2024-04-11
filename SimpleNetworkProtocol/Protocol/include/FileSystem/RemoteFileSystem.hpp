#pragma once

#include <string_view>

#include "FileSystem.hpp"
#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class RemoteFileSystem : public FileSystem::FileSystem {
public:
    explicit RemoteFileSystem(std::string_view mountPoint);
    static ITaskScheduler* scheduler;

private:
    static void FuseGetattr(fuse_req_t req, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseLookup(fuse_req_t req, fuse_ino_t parentId, const char* name);
    static void FuseOpendir(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseOpen(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseForget(fuse_req_t request, fuse_ino_t inode, std::uint64_t nlookup);
    static void FuseForgetmulti(fuse_req_t request, size_t count, fuse_forget_data* forgets);
    static void FuseRelease(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseReleaseDir(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo);
    static void FuseRead(fuse_req_t request, fuse_ino_t inode, size_t size, off_t off, fuse_file_info* fileInfo);
};

} // namespace FastTransport::TaskQueue
