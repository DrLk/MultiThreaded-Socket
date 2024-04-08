#include "ResponseReleaseInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJobIn] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseReleaseInJob::ExecuteMain(std::stop_token /*stop*/, Writer& /*writer*/)
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
