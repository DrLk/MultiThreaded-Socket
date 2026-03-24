#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "FileCache/PinnedFuseBufVec.hpp"

namespace FastTransport::FileSystem {
class NativeFile;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileCache {

// Handles a cross-block FUSE read where block B is in memory and block B+1 is on disk.
// Assembles a combined fuse_bufvec: memory entries for B followed by a disk fd entry for B+1.
class PrefixDiskReadFileCacheJob : public TaskQueue::DiskJob {
public:
    PrefixDiskReadFileCacheJob(fuse_req_t request, FileSystem::FileCache::PinnedFuseBufVec prefixData,
        std::shared_ptr<FileSystem::NativeFile> file, off_t diskOffset, size_t diskSize);
    Data ExecuteDisk(TaskQueue::ITaskScheduler& scheduler, Data&& free) override;

private:
    fuse_req_t _request;
    FileSystem::FileCache::PinnedFuseBufVec _prefixData;
    std::shared_ptr<FileSystem::NativeFile> _file;
    off_t _diskOffset;
    size_t _diskSize;
};

} // namespace FastTransport::FileCache
