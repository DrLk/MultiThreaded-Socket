#include "ResponseReadFileInJob.hpp"

#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include <stop_token>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

void ResponseReadFileInJob::GetBuffer(const Message& message)
{
    fuse_bufvec buf{};
    buf.count = message.size();
}

ResponseInFuseNetworkJob::Message ResponseReadFileInJob::ExecuteResponse(std::stop_token  /*stop*/, FileTree&  /*fileTree*/)
{
    TRACER() << "Execute";
    fuse_req_t req = nullptr;
    int error = 0;
    Message data;
    _reader >> req;
    _reader >> error;
    _reader >> data;

    if (error != 0) {
        fuse_reply_err(req, error);
        return {};
    }

    struct fuse_bufvec* bufv = nullptr;
    std::size_t len = sizeof(fuse_bufvec) + sizeof(fuse_buf) * (data.size() - 1);
    bufv = static_cast<fuse_bufvec*>(calloc(1, len));
    if (bufv) {
        int i = 0;
        bufv->count = data.size();
        for (auto& packet : data) {
            bufv->buf[i].mem = packet->GetPayload().data();
            bufv->buf[i++].size = packet->GetPayload().size();
        }
    }

    fuse_reply_data(req, bufv, FUSE_BUF_SPLICE_MOVE);
    free(bufv);
    return _reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
