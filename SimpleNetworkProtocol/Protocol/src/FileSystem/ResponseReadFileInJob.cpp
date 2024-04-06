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

FuseNetworkJob::Message ResponseReadFileInJob::ExecuteMain(std::stop_token  /*stop*/, Writer&  /*writer*/)
{
    TRACER() << "Execute";
    fuse_req_t req = nullptr;
    ssize_t size = 0;
    Message data;
    _reader >> req;
    _reader >> size;
    _reader >> data;

    struct fuse_bufvec* bufv = nullptr;
    std::size_t len = sizeof(struct fuse_bufvec) + sizeof(struct fuse_buf) * (data.size() - 1);
    bufv = static_cast<struct fuse_bufvec*>(calloc(1, len));
    if (bufv) {
        int i = 0;
        bufv->count = data.size();
        for (auto& v : data) {
            bufv->buf[i].mem = v->GetPayload().data();
            bufv->buf[i++].size = v->GetPayload().size();
        }
    }

    fuse_reply_data(req, bufv, FUSE_BUF_SPLICE_MOVE);
    free(bufv);
    return _reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
