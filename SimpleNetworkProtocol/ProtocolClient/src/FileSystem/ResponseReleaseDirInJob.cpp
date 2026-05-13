#include "ResponseReleaseDirInJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FuseRequestTracker.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseDirInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReleaseDirInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseReleaseDirInJob::ExecuteResponse");
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    FileHandle* handle = nullptr; // NOLINT(misc-const-correctness)
    reader >> request;
    reader >> error;
    reader >> handle;

    TRACER() << "Execute"
             << " request: " << request;

    if (error != 0) {
        FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(request), error));
        return {};
    }

    delete handle; // NOLINT(cppcoreguidelines-owning-memory)

    FUSE_ASSERT_REPLY(fuse_reply_err(FUSE_UNTRACK(request), error));

    return {};
}

} // namespace FastTransport::TaskQueue
