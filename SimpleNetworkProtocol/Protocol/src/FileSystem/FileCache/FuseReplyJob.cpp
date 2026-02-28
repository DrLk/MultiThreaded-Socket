#include "FileCache/FuseReplyJob.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[FuseReplyJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileCache {

FuseReplyJob::FuseReplyJob(fuse_req_t request, Message&& packets)
    : _request(request)
    , _packets(std::move(packets))
    , _buffer(nullptr)
{
}

void FuseReplyJob::PrepareBuffer()
{
    // Allocate space for all buffers
    size_t bufSize = sizeof(fuse_bufvec) + (sizeof(fuse_buf) * (_packets.size() - 1));
    _bufferMem = std::make_unique<char[]>(bufSize);
    _buffer = reinterpret_cast<fuse_bufvec*>(_bufferMem.get());
    
    _buffer->count = _packets.size();
    _buffer->idx = 0;
    _buffer->off = 0;

    // Fill buffer info for each packet
    int idx = 0;
    for (auto& packet : _packets) {
        auto& buf = _buffer->buf[idx++];
        buf.mem = packet->GetPayload().data();
        buf.size = packet->GetPayload().size();
        buf.flags = static_cast<fuse_buf_flags>(0);
        buf.pos = 0;
    }
}

FuseReplyJob::Message FuseReplyJob::ExecuteResponse(TaskQueue::ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    TRACER() << "Execute request=" << _request;
    PrepareBuffer();
    fuse_reply_data(_request, _buffer, fuse_buf_copy_flags::FUSE_BUF_SPLICE_MOVE);
    return std::move(_packets);
}

} // namespace FastTransport::FileCache 
