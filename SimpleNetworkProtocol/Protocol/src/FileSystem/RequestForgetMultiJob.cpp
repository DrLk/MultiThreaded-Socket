#include "RequestForgetMultiJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "ServerInode.hpp"

#define TRACER() LOGGER() << "[RequestForgetMultiJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestForgetMultiJob::RequestForgetMultiJob(fuse_req_t request, std::span<fuse_forget_data> forgets)
    : _request(request)
    , _forgets(forgets.begin(), forgets.end())
{
    // Translate client inodes (= Leaf*) to server inodes synchronously, while we
    // are still on the FUSE thread that delivered the forget. By the time
    // ExecuteMain runs on a worker, the Leaf may have been destroyed.
    for (auto& forget : _forgets) {
        forget.ino = ToServerInode(forget.ino);
    }
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestForgetMultiJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    ZoneScopedN("RequestForgetMultiJob::ExecuteMain");
    TRACER() << "Execute"
             << " request: " << _request
             << " size: " << _forgets.size()
             << " forgets: " << _forgets.data();

    writer << MessageType::RequestForgetMulti;
    writer << _request;
    writer << std::span(_forgets);

    return {};
}

} // namespace FastTransport::TaskQueue
