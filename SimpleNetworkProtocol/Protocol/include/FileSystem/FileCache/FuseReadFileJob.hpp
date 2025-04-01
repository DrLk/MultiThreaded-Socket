#pragma once

#include "CacheTreeJob.hpp"

namespace FastTransport::FileCache {

class FuseReadFileJob : public TaskQueue::CacheTreeJob {
public:
    FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile);
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    FileSystem::RemoteFileHandle* _remoteFile;
};
} // namespace FastTransport::FileCache
