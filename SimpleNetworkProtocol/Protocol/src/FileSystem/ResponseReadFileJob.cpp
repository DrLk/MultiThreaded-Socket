#include "ResponseReadFileJob.hpp"

#include <stop_token>
#include <sys/uio.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReadFileJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{

    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    int file = 0;
    auto& reader = GetReader();
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> file;

    TRACER() << "Execute"
             << " request=" << request;

    writer << MessageType::ResponseRead;
    writer << request;
    writer << inode;
    writer << size;
    writer << offset;

    Message dataPackets = writer.GetDataPackets(200);
    std::array<iovec, 200> iovecs {};

    auto packet = dataPackets.begin();
    for (auto& iovec : iovecs) {
        iovec.iov_base = (*packet)->GetPayload().data();
        iovec.iov_len = (*packet)->GetPayload().size();
        assert(1300 == (*packet)->GetPayload().size());
        ++packet;
    }

    int error = 0;
    const ssize_t readed = preadv(file, iovecs.data(), iovecs.size(), offset);

    if (readed == -1) {
        TRACER() << "preadv failed";
        error = errno;
        writer << error;
        return {};
    }

    writer << error;
    writer << std::move(dataPackets);

    return {};
}

} // namespace FastTransport::TaskQueue
