#include "ResponseReadFileInJob.hpp"

#include <algorithm>
#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include <stop_token>
#include <sys/types.h>

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
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    int error = 0;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> error;

    TRACER() << "Execute"
             << " request=" << request
             << " inode=" << inode
             << " size=" << size
             << " offset=" << offset
             << " error=" << error;

    if (error != 0) {
        fuse_reply_err(request, error);
        return {};
    }

    Message data;
    reader >> data;

    std::size_t len = sizeof(fuse_bufvec) + sizeof(fuse_buf) * (data.size() - 1);
    std::unique_ptr<fuse_bufvec> buffVector(static_cast<fuse_bufvec*>(reinterpret_cast<fuse_bufvec*>(new char[len]))); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    int index = 0;
    size_t count = 0;

    /* buffVector->off = offset; */
    buffVector->off = 0;
    buffVector->idx = 0;
    for (auto& packet : data) {
        auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = packet->GetPayload().data();
        buffer.size = std::min(packet->GetPayload().size(), size);
        buffer.flags = static_cast<fuse_buf_flags>(0);
        buffer.pos = 0;
        buffer.fd = 0;

        count++;
        if (size <= packet->GetPayload().size()) {
            break;
        }

        size -= packet->GetPayload().size();
        offset += static_cast<off_t>(packet->GetPayload().size());
    }
    buffVector->count = count;

    fuse_reply_data(request, buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
    return data;
}

} // namespace FastTransport::TaskQueue
