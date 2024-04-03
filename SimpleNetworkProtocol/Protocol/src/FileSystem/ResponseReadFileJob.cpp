#include "ResponseReadFileJob.hpp"

#include <stop_token>
#include <vector>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseReadFileJob::ResponseReadFileJob(Reader&& reader)
    : FuseNetworkJob()
    , _reader(std::move(reader))
{
}

FuseNetworkJob::Message ResponseReadFileJob::ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Writer& writer)
{
    TRACER() << "Execute";

    fuse_req_t request;
    int file;
    size_t size;
    off_t offset;
    _reader >> request;
    _reader >> file;
    _reader >> size;
    _reader >> offset;


    return _reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
