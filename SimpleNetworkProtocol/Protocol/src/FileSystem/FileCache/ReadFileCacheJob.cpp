#include "ReadFileCacheJob.hpp"

#include <bit>
#include <memory>

#include <fuse3/fuse_lowlevel.h>

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

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::FileCache::PinnedFuseBufVec prefixData,
    FileSystem::NativeFile::Ptr file, off_t diskOffset, size_t diskSize)
    : _request(request)
    , _file(std::move(file))
    , _size(0) // signals prefix+disk mode
    , _offset(diskOffset)
    , _prefixData(std::move(prefixData))
    , _diskSize(diskSize)
{
}

TaskQueue::DiskJob::Data ReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& /*scheduler*/, Protocol::IPacket::List&& free)
{
    // Memory-prefix + disk-suffix mode: in-memory data for block B followed by disk data for block B+1.
    if (_prefixData.HasData()) {
        const std::size_t prefixCount = _prefixData.bufvec->count;
        const std::size_t totalCount = prefixCount + 1;
        const std::size_t allocSize = sizeof(fuse_bufvec) + (prefixCount * sizeof(fuse_buf));
        std::unique_ptr<fuse_bufvec> combined(reinterpret_cast<fuse_bufvec*>(new char[allocSize])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        combined->count = totalCount;
        combined->idx = 0;
        combined->off = 0;

        // Entries 0..n-1: memory portions from block B.
        for (std::size_t i = 0; i < prefixCount; ++i) {
            combined->buf[i] = _prefixData.bufvec->buf[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        // Entry n: disk portion for block B+1.
        combined->buf[prefixCount] = {}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        combined->buf[prefixCount].size = _diskSize; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        combined->buf[prefixCount].flags = std::bit_cast<fuse_buf_flags>( // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            static_cast<unsigned int>(FUSE_BUF_IS_FD) | static_cast<unsigned int>(FUSE_BUF_FD_SEEK));
        combined->buf[prefixCount].fd = _file->GetHandle(); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        combined->buf[prefixCount].pos = _offset; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

        fuse_reply_data(_request, combined.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
        // _prefixData destructs here: pin unpins Ranges → blocks can now be evicted to disk.
        return std::move(free);
    }

    if (!_appendData.HasData()) {
        // Zero-copy path: splice directly from cache file fd to FUSE device.
        // fuse_reply_data is thread-safe in libfuse3.
        fuse_bufvec buf = FUSE_BUFVEC_INIT(_size); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buf.buf[0].flags = std::bit_cast<fuse_buf_flags>(static_cast<unsigned int>(FUSE_BUF_IS_FD) | static_cast<unsigned int>(FUSE_BUF_FD_SEEK));
        buf.buf[0].fd = _file->GetHandle();
        buf.buf[0].pos = _offset;
        fuse_reply_data(_request, &buf, FUSE_BUF_SPLICE_MOVE);
        return std::move(free);
    }

    // Cross-block case: first part from disk fd (splice, zero-copy), second part from
    // pinned in-memory Ranges (direct pointers into Leaf packet payloads, no copy).
    // _appendData.pin keeps the backing Ranges alive until this function returns.
    size_t appendTotalSize = 0;
    for (std::size_t i = 0; i < _appendData.bufvec->count; ++i) {
        appendTotalSize += _appendData.bufvec->buf[i].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    const size_t diskSize = _size - appendTotalSize;
    const std::size_t totalCount = 1 + _appendData.bufvec->count;
    const std::size_t allocSize = sizeof(fuse_bufvec) + ((_appendData.bufvec->count) * sizeof(fuse_buf));
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
    for (std::size_t i = 0; i < _appendData.bufvec->count; ++i) {
        combined->buf[1 + i] = _appendData.bufvec->buf[i]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    fuse_reply_data(_request, combined.get(), FUSE_BUF_SPLICE_MOVE);
    // _appendData destructs here: pin unpins Ranges → blocks can now be evicted to disk.
    return std::move(free);
}

} // namespace FastTransport::FileCache
