#include "ResponseReadFileJob.hpp"

#include <stop_token>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

FuseNetworkJob::Message ResponseReadFileJob::ExecuteMain(std::stop_token  /*stop*/, Writer&  /*writer*/)
{
    TRACER() << "Execute";

    fuse_req_t request = nullptr;
    int file = 0;
    size_t size = 0;
    off_t offset = 0;
    auto& reader = GetReader();
    reader >> request;
    reader >> file;
    reader >> size;
    reader >> offset;


    return reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
