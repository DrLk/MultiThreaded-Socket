#include "FileCache/FuseReplyJob.hpp"
#include <Tracy.hpp>

#include <bit>

#include "ITaskScheduler.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[FuseReplyJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

FuseReplyJob::FuseReplyJob(fuse_req_t request, Message&& packets)
    : _request(request)
    , _packets(std::move(packets))
{
}

void FuseReplyJob::PrepareBuffer()
{
    // Allocate space for all buffers
    const size_t bufSize = sizeof(fuse_bufvec) + (sizeof(fuse_buf) * (_packets.size() - 1));
    _bufferMem.resize(bufSize);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,bugprone-casting-through-void)
    _buffer = reinterpret_cast<fuse_bufvec*>(_bufferMem.data());

    _buffer->count = _packets.size();
    _buffer->idx = 0;
    _buffer->off = 0;

    // Fill buffer info for each packet
    int idx = 0;
    for (auto& packet : _packets) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto& buf = _buffer->buf[idx++];
        buf = fuse_buf { .flags = std::bit_cast<fuse_buf_flags>(0) };
        buf.mem = packet->GetPayload().data();
        buf.size = packet->GetPayload().size();
    }
}

FuseReplyJob::Message FuseReplyJob::ExecuteResponse(TaskQueue::ITaskScheduler& scheduler, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("FuseReplyJob::ExecuteResponse");
    TRACER() << "Execute request=" << _request;
    PrepareBuffer();
    fuse_reply_data(_request, _buffer, fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
    scheduler.ReturnFreeDiskPackets(std::move(_packets));
    return {};
}

} // namespace FastTransport::FileCache
