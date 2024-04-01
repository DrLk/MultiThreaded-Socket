#include "ResponseReadFileJob.hpp"

#include <stop_token>
#include <sys/types.h>
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

    size_t blockSize = 1500;
    int blocks = (int)size / blockSize;
    if (size % blockSize != 0) {
        blocks++;
    }

    std::vector<iovec> iovecs(blocks);
    iovecs[0].iov_base = nullptr;

    int result = preadv(file, iovecs.data(), blocks, offset);
    assert(result != -1);

    return _reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
