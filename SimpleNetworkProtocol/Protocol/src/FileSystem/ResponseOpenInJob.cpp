#include "ResponseOpenInJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseOpenInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseOpenInJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& /*fileTree*/)
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

    fuse_file_info* fileInfo = nullptr;
    reader >> fileInfo;
    reader >> fileInfo->fh;

    fuse_reply_open(request, fileInfo);
    return {};
}

} // namespace FastTransport::TaskQueue
