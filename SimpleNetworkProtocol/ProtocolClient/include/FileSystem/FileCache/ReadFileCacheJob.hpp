#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "FileCache/PinnedFuseBufVec.hpp"

namespace FastTransport::FileSystem {
class NativeFile;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

class ReadFileCacheJob : public TaskQueue::DiskJob {
public:
    ReadFileCacheJob(fuse_req_t request, std::shared_ptr<FileSystem::NativeFile> file, size_t size, off_t offset);
    ReadFileCacheJob(fuse_req_t request, std::shared_ptr<FileSystem::NativeFile> file, size_t size, off_t offset,
        FileSystem::FileCache::PinnedFuseBufVec appendData);
    Data ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free) override;

private:
    fuse_req_t _request;
    std::shared_ptr<FileSystem::NativeFile> _file;
    size_t _size;
    off_t _offset;
    FileSystem::FileCache::PinnedFuseBufVec _appendData;
};
} // namespace FastTransport::FileCache
