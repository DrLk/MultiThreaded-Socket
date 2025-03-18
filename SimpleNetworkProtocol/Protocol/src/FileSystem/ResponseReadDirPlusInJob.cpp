#include "ResponseReadDirPlusInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirPlusJobIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadDirPlusInJob::ExecuteResponse(ITaskScheduler&  /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    Message data;

    reader >> request;

    TRACER() << "Execute"
             << " request: " << request;

    reader >> data;

    const std::size_t length = sizeof(fuse_bufvec) + (sizeof(fuse_buf) * (data.size() - 1));
    std::unique_ptr<fuse_bufvec> buffVector(reinterpret_cast<fuse_bufvec*>(new char[length])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    int index = 0;

    buffVector->off = 0;
    buffVector->idx = 0;
    for (auto& packet : data) {
        auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = packet->GetPayload().data();
        buffer.size = packet->GetPayload().size();
        buffer.flags = static_cast<fuse_buf_flags>(0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buffer.pos = 0;
    }
    buffVector->count = data.size();

    fuse_reply_data(request, buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_SPLICE_MOVE);

    return data;
}

} // namespace FastTransport::TaskQueue
