#include "RequestReadFileJob.hpp"

#include "File.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReadFileJob::RequestReadFileJob(File& file, fuse_req_t request, size_t size, off_t off)
    : _file(file)
    , _request(request)
    , _size(size)
    , _off(off)
{
}

FuseNetworkJob::Message RequestReadFileJob::ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Writer& writer)
{
    TRACER() << "Execute"
             << " size: " << _size
             << " off: " << _off;

    writer << MessageType::RequestReadFile;
    writer << _request;
    writer << _size;
    writer << _off;

    return Message();
}

} // namespace FastTransport::TaskQueue
