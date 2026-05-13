#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "CacheTreeJob.hpp"
#include "FileCache/Range.hpp"

namespace FastTransport::FileCache {

class FuseReadFileJob : public TaskQueue::CacheTreeJob {
public:
    FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile);
    FuseReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, FileSystem::RemoteFileHandle* remoteFile,
        FileSystem::FileCache::RangePin arrivedBlockPin, FileSystem::FileCache::RangePin requestBlockPin);
    ~FuseReadFileJob() override;
    FuseReadFileJob(const FuseReadFileJob&) = delete;
    FuseReadFileJob(FuseReadFileJob&&) = delete;
    FuseReadFileJob& operator=(const FuseReadFileJob&) = delete;
    FuseReadFileJob& operator=(FuseReadFileJob&&) = delete;
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;
    [[nodiscard]] std::uint64_t GetInode() const noexcept override { return _inode; }

private:
    void FetchBlock(FileSystem::Leaf& leaf, size_t blockIndex, TaskQueue::ITaskScheduler& scheduler);
    void TriggerPrefetch(FileSystem::Leaf& leaf, size_t blockIndex, TaskQueue::ITaskScheduler& scheduler);
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    FileSystem::RemoteFileHandle* _remoteFile;
    // Pins to prevent eviction before ExecuteCachedTree runs.
    // _arrivedBlockPin: the block that just arrived (triggered scheduling this job).
    // _requestBlockPin: the block containing _offset (may differ from arrived block).
    FileSystem::FileCache::RangePin _arrivedBlockPin;
    FileSystem::FileCache::RangePin _requestBlockPin;
};
} // namespace FastTransport::FileCache
