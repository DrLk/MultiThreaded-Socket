#include "ResponseOpenInJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FuseRequestTracker.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseOpenInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseOpenInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseOpenInJob::ExecuteResponse");
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    reader >> request;
    reader >> error;

    TRACER() << "Execute"
             << " request: " << request;

    if (error != 0) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(request), error));
        return {};
    }

    fuse_file_info fileInfo {};
    reader >> fileInfo;
    fileInfo.fh = reinterpret_cast<std::uint64_t>(new FileHandle()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-owning-memory)
    reader >> GetFileHandle(&fileInfo).remoteFile;

    FUSE_ASSERT_REPLY(fuse_reply_open(FUSE_UNTRACK(request), &fileInfo));
    return {};
}

} // namespace FastTransport::TaskQueue
