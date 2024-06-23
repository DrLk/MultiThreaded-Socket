#pragma once

#include "CacheTreeJob.hpp"

namespace FastTransport::FileCache {

class ReadFileCacheJob : public TaskQueue::CacheTreeJob {
public:
    ReadFileCacheJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo);
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    fuse_file_info* _fileInfo;
};
} // namespace FastTransport::FileCache
