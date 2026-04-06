#include "ReadFileCacheJob.hpp"
#include <Tracy.hpp>

#include <bit>
#include <memory>

#include <fuse3/fuse_lowlevel.h>

#include "FuseRequestTracker.hpp"
#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::NativeFile::Ptr file, size_t size, off_t offset)
    : _request(request)
    , _file(std::move(file))
    , _size(size)
    , _offset(offset)
{
}

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::NativeFile::Ptr file, size_t size, off_t offset,
    FileSystem::FileCache::PinnedFuseBufVec appendData)
    : _request(request)
    , _file(std::move(file))
    , _size(size)
    , _offset(offset)
    , _appendData(std::move(appendData))
{
}

TaskQueue::DiskJob::Data ReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& /*scheduler*/, Protocol::IPacket::List&& free)
{
    ZoneScopedN("ReadFileCacheJob::ExecuteDisk");
    if (!_appendData.HasData()) {
        // Zero-copy path: splice directly from cache file fd to FUSE device.
        // fuse_reply_data is thread-safe in libfuse3.
        fuse_bufvec buf = FUSE_BUFVEC_INIT(_size); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buf.buf[0].flags = std::bit_cast<fuse_buf_flags>(static_cast<unsigned int>(FUSE_BUF_IS_FD) | static_cast<unsigned int>(FUSE_BUF_FD_SEEK));
        buf.buf[0].fd = _file->GetHandle();
        buf.buf[0].pos = _offset;
        FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(_request), &buf, FUSE_BUF_SPLICE_MOVE));
        return std::move(free);
    }

    // Cross-block case: first part from disk fd (splice, zero-copy), second part from
    // pinned in-memory Ranges (direct pointers into Leaf packet payloads, no copy).
    // _appendData.pin keeps the backing Ranges alive until this function returns.
    size_t appendTotalSize = 0;
    for (std::size_t i = 0; i < _appendData->count; ++i) {
        appendTotalSize += _appendData->buf[i].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    const size_t diskSize = _size - appendTotalSize;
    const std::size_t totalCount = 1 + _appendData->count;
    const std::size_t allocSize = sizeof(fuse_bufvec) + ((_appendData->count) * sizeof(fuse_buf));
    std::unique_ptr<fuse_bufvec> combined(reinterpret_cast<fuse_bufvec*>(new char[allocSize])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    combined->count = totalCount;
    combined->idx = 0;
    combined->off = 0;

    // Entry 0: disk portion via fd splice.
    combined->buf[0] = {}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    combined->buf[0].size = diskSize; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    combined->buf[0].flags = std::bit_cast<fuse_buf_flags>( // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        static_cast<unsigned int>(FUSE_BUF_IS_FD) | static_cast<unsigned int>(FUSE_BUF_FD_SEEK));
    combined->buf[0].fd = _file->GetHandle(); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    combined->buf[0].pos = _offset; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

    // Entries 1..n: memory portions — direct pointers into pinned Leaf packet payloads.
    for (std::size_t i = 0; i < _appendData->count; ++i) {
        combined->buf[1 + i] = _appendData->buf[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(_request), combined.get(), FUSE_BUF_SPLICE_MOVE));
    // _appendData destructs here: pin unpins Ranges → blocks can now be evicted to disk.
    return std::move(free);
}

} // namespace FastTransport::FileCache
