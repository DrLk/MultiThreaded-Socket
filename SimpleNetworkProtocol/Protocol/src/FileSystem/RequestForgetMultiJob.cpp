#include "RequestForgetMultiJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestForgetMultiJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestForgetMultiJob::RequestForgetMultiJob(fuse_req_t request, std::span<fuse_forget_data> forgets)
    : _request(request)
    , _forgets(forgets.begin(), forgets.end())
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestForgetMultiJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
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
