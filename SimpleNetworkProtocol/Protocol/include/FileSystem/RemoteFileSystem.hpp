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
};

} // namespace FastTransport::TaskQueue
