#include "RequestLookupJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestLookupJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestLookupJob::RequestLookupJob(fuse_req_t request, fuse_ino_t parent, std::string&& name)
    : _request(request)
    , _parent(parent)
    , _name(std::move(name))
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestLookupJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestLookup;
    writer << _request;
    writer << _parent;
    writer << _name;

    return {};
}

} // namespace FastTransport::TaskQueue
