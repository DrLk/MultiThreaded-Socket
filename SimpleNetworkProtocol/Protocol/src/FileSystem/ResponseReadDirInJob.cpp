#include "ResponseReadDirInJob.hpp"
#include <Tracy.hpp>

#include <bit>
#include <cstddef>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FuseRequestTracker.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadDirInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseReadDirInJob::ExecuteResponse");
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
        buffer.flags = std::bit_cast<fuse_buf_flags>(0);
        buffer.pos = 0;
    }
    buffVector->count = data.size();

    FUSE_ASSERT_REPLY(fuse_reply_data(FUSE_UNTRACK(request), buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_SPLICE_MOVE));

    return reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
