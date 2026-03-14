#include "ReadFileCacheJob.hpp"

#include <algorithm>
#include <vector>

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "NativeFile.hpp"
#include <fuse3/fuse_lowlevel.h>

namespace FastTransport::FileCache {

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::NativeFile::Ptr file, size_t size, off_t offset)
    : ReadFileCacheJob(request, std::move(file), size, offset, {})
{
}

ReadFileCacheJob::ReadFileCacheJob(fuse_req_t request, FileSystem::NativeFile::Ptr file, size_t size, off_t offset, std::vector<char> appendData)
    : _request(request)
    , _file(std::move(file))
    , _size(size)
    , _offset(offset)
    , _appendData(std::move(appendData))
{
}

TaskQueue::DiskJob::Data ReadFileCacheJob::ExecuteDisk(TaskQueue::ITaskScheduler& /*scheduler*/, Protocol::IPacket::List&& free)
{
    // Capture full payload size before Read() may truncate the last packet.
    const size_t fullPayloadSize = free.empty() ? 0 : free.front()->GetPayload().size();
    Data packets = _file->Read(free, _size, _offset);

    // Collect up to _size bytes into a contiguous buffer for fuse_reply_buf.
    // NativeFile::Read may return more bytes than requested (last packet padding),
    // so cap at _size to avoid EINVAL from the kernel.
    // fuse_reply_buf is thread-safe in libfuse3.
    std::vector<char> buf(_size);
    size_t copied = 0;
    for (const auto& packet : packets) {
        if (copied >= _size) {
            break;
        }
        const auto payload = packet->GetPayload();
        const size_t toCopy = std::min(payload.size(), _size - copied);
        std::transform(payload.begin(), std::next(payload.begin(), static_cast<std::ptrdiff_t>(toCopy)),
            std::next(buf.begin(), static_cast<std::ptrdiff_t>(copied)),
            [](std::byte byte) { return std::to_integer<char>(byte); });
        copied += toCopy;
    }

    // If the cache file ended before _size (cross-block read), append pre-fetched memory data.
    if (copied < _size && !_appendData.empty()) {
        const size_t toAppend = std::min(_appendData.size(), _size - copied);
        std::copy_n(_appendData.begin(), static_cast<std::ptrdiff_t>(toAppend),
            std::next(buf.begin(), static_cast<std::ptrdiff_t>(copied)));
        copied += toAppend;
    }

    fuse_reply_buf(_request, buf.data(), copied);

    // NativeFile::Read may have truncated the last packet's payload size for a short read.
    // Reset all packets to full capacity before returning them to the pool.
    if (fullPayloadSize > 0) {
        for (auto& packet : packets) {
            if (packet->GetPayload().size() != fullPayloadSize) {
                packet->SetPayloadSize(fullPayloadSize);
            }
        }
    }

    // Return all packets (used + remaining free) immediately to the disk pool.
    packets.splice(std::move(free));
    return packets;
}

} // namespace FastTransport::FileCache
