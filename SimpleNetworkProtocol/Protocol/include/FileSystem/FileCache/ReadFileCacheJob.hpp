#pragma once

#include "DiskJob.hpp"
#include <fuse3/fuse_lowlevel.h>

namespace FastTransport::FileSystem {
class NativeFile;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

class ReadFileCacheJob : public TaskQueue::DiskJob {
public:
    ReadFileCacheJob(fuse_req_t request,std::shared_ptr<FileSystem::NativeFile>& file, size_t size, off_t offset);
    Data ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free) override;

private:
    fuse_req_t _request;
    std::shared_ptr<FileSystem::NativeFile> _file;
    size_t _size;
    off_t _offset;
};
} // namespace FastTransport::FileCache
