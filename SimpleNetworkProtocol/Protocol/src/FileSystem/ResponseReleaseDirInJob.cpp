#include "ResponseReleaseDirInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseDirInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReleaseDirInJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    reader >> request;
    reader >> error;

    TRACER() << "Execute"
             << " request: " << request;

    if (error != 0) {
        fuse_reply_err(request, error);
        return {};
    }

    fuse_reply_err(request, error);

    return {};
}

} // namespace FastTransport::TaskQueue
