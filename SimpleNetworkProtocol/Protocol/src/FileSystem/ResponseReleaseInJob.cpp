#include "ResponseReleaseInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"
#include "ResponseInFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReleaseInJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    FileHandle* handle = nullptr;
    reader >> request;
    reader >> error;
    reader >> handle;

    TRACER() << "Execute"
             << " request: " << request;

    if (error != 0) {
        fuse_reply_err(request, error);
        return {};
    }

    delete handle; // NOLINT(cppcoreguidelines-owning-memory)

    fuse_reply_err(request, error);

    return {};
}

} // namespace FastTransport::TaskQueue
