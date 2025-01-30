#include "ResponseReadFileInJob.hpp"

#include <algorithm>
#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include <stop_token>
#include <sys/types.h>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[ResponseReadFileInJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseInFuseNetworkJob::Message ResponseReadFileInJob::ExecuteResponse(std::stop_token /*stop*/, FileTree& fileTree)
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

    const size_t readed = ((data.size() - 1) * data.front()->GetPayload().size()) + data.back()->GetPayload().size();
    size_t replySize = std::min(readed, size);
    const std::size_t length = sizeof(fuse_bufvec) + (sizeof(fuse_buf) * (data.size() - 1));
    std::unique_ptr<fuse_bufvec> buffVector(reinterpret_cast<fuse_bufvec*>(new char[length])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    int index = 0;
    size_t count = 0;

    buffVector->off = 0;
    buffVector->idx = 0;
    for (auto& packet : data) {
        auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = packet->GetPayload().data();
        buffer.size = std::min(packet->GetPayload().size(), replySize);
        buffer.flags = static_cast<fuse_buf_flags>(0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buffer.pos = 0;
        buffer.fd = 0;

        count++;
        if (replySize <= packet->GetPayload().size()) {
            break;
        }

        replySize -= packet->GetPayload().size();
    }
    buffVector->count = count;

    fuse_reply_data(request, buffVector.get(), fuse_buf_copy_flags::FUSE_BUF_NO_SPLICE);
    return GetLeaf(inode, fileTree).AddData(offset, readed, std::move(data));
}

} // namespace FastTransport::TaskQueue
