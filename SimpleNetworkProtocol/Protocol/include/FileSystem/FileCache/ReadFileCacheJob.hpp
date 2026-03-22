#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "FileCache/Range.hpp"

namespace FastTransport::FileSystem {
class NativeFile;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

class ReadFileCacheJob : public TaskQueue::DiskJob {
public:
    ReadFileCacheJob(fuse_req_t request, std::shared_ptr<FileSystem::NativeFile> file, size_t size, off_t offset);
    ReadFileCacheJob(fuse_req_t request, std::shared_ptr<FileSystem::NativeFile> file, size_t size, off_t offset,
        FileSystem::FileCache::PinnedFuseBufVec appendData);
    // Memory-prefix + disk-suffix constructor: in-memory data for block B followed by
    // a disk read for block B+1 portion (diskOffset..diskOffset+diskSize).
    ReadFileCacheJob(fuse_req_t request, FileSystem::FileCache::PinnedFuseBufVec prefixData,
        std::shared_ptr<FileSystem::NativeFile> file, off_t diskOffset, size_t diskSize);
    Data ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free) override;

private:
    fuse_req_t _request;
    std::shared_ptr<FileSystem::NativeFile> _file;
    size_t _size;
    off_t _offset;
    FileSystem::FileCache::PinnedFuseBufVec _appendData;
    // For memory-prefix + disk-suffix mode (_size == 0 signals this mode).
    FileSystem::FileCache::PinnedFuseBufVec _prefixData;
    size_t _diskSize { 0 };
};
} // namespace FastTransport::FileCache
