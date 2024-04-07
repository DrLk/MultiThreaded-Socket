#include "ResponseForgetMultiInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseForgetMultiInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseForgetMultiInJob::ExecuteMain(std::stop_token /*stop*/, Writer& /*writer*/)
{

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    reader >> request;

    TRACER() << "Execute"
             << " request: " << request;

    fuse_reply_none(request);
    return {};
}

} // namespace FastTransport::TaskQueue
