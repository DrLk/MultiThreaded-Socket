#pragma once

#include "CacheTreeJob.hpp"
#include "FileCache/Range.hpp"

namespace FastTransport::FileCache {

class FuseReadFileJob : public TaskQueue::CacheTreeJob {
public:
    FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile);
    FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile,
        FileSystem::FileCache::PinnedFuseBufVec arrivedBlockPin, FileSystem::FileCache::PinnedFuseBufVec requestBlockPin);
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    FileSystem::RemoteFileHandle* _remoteFile;
    // Pins to prevent eviction before ExecuteCachedTree runs.
    // _arrivedBlockPin: the block that just arrived (triggered scheduling this job).
    // _requestBlockPin: the block containing _offset (may differ from arrived block).
    FileSystem::FileCache::PinnedFuseBufVec _arrivedBlockPin;
    FileSystem::FileCache::PinnedFuseBufVec _requestBlockPin;
};
} // namespace FastTransport::FileCache
