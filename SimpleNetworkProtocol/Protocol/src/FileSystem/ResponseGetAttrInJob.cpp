#include "ResponseGetAttrInJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FuseRequestTracker.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseGetAttrInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseGetAttrInJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*fileTree*/)
{
    ZoneScopedN("ResponseGetAttrInJob::ExecuteResponse");
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

    struct stat stbuf {};
    reader >> stbuf.st_ino;
    reader >> stbuf.st_mode;
    reader >> stbuf.st_nlink;
    reader >> stbuf.st_size;
    reader >> stbuf.st_uid;
    reader >> stbuf.st_gid;
    reader >> stbuf.st_mtim;

    error = FUSE_ASSERT_REPLY(fuse_reply_attr(FUSE_UNTRACK(request), &stbuf, 1.0));

    TRACER() << "Reply with attributes"
             << " request: " << request
             << " error: " << error;

    return reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
