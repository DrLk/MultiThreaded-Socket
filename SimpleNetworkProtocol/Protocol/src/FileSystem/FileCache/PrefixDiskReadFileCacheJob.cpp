#include "PrefixDiskReadFileCacheJob.hpp"
#include <Tracy.hpp>

#include <bit>
#include <memory>

#include <fuse3/fuse_lowlevel.h>

#include "FuseRequestTracker.hpp"
#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileCache {

PrefixDiskReadFileCacheJob::PrefixDiskReadFileCacheJob(fuse_req_t request,
    FileSystem::FileCache::PinnedFuseBufVec prefixData,
    std::shared_ptr<FileSystem::NativeFile> file, off_t diskOffset, size_t diskSize)
    : _request(request)
    , _prefixData(std::move(prefixData))
    , _file(std::move(file))
    , _diskOffset(diskOffset)
    , _diskSize(diskSize)
{
}

TaskQueue::DiskJob::Data PrefixDiskReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& /*scheduler*/, Protocol::IPacket::List&& free)
{
    ZoneScopedN("PrefixDiskReadFileCacheJob::ExecuteDisk");
    const std::size_t prefixCount = _prefixData->count;
    const std::size_t totalCount = prefixCount + 1;
    const std::size_t allocSize = sizeof(fuse_bufvec) + (prefixCount * sizeof(fuse_buf));
    std::unique_ptr<fuse_bufvec> combined(reinterpret_cast<fuse_bufvec*>(new char[allocSize])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    combined->count = totalCount;
    combined->idx = 0;
    combined->off = 0;

    // Entries 0..n-1: memory portions from block B.
    for (std::size_t i = 0; i < prefixCount; ++i) {
        combined->buf[i] = _prefixData->buf[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    // Entry n: disk portion for block B+1.
    combined->buf[prefixCount] = {}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index,bugprone-invalid-enum-default-initialization)
    combined->buf[prefixCount].size = _diskSize; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    combined->buf[prefixCount].flags = std::bit_cast<fuse_buf_flags>( // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        static_cast<unsigned int>(FUSE_BUF_IS_FD) | static_cast<unsigned int>(FUSE_BUF_FD_SEEK));
    combined->buf[prefixCount].fd = _file->GetHandle(); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    combined->buf[prefixCount].pos = _diskOffset; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(_request), combined.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE));
    // _prefixData destructs here: pin unpins Ranges → blocks can now be evicted to disk.
    return std::move(free);
}

} // namespace FastTransport::FileCache
