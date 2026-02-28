#pragma once

#include "ResponseInFuseNetworkJob.hpp"
#include <fuse3/fuse_lowlevel.h>

namespace FastTransport::FileCache {

class FuseReplyJob : public TaskQueue::ResponseInFuseNetworkJob {
public:
    using Message = Protocol::IPacket::List;

    FuseReplyJob(fuse_req_t request, Message&& packets);
    Message ExecuteResponse(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& fileTree) override;

private:
    fuse_req_t _request;
    Message _packets;
    std::unique_ptr<char[]> _bufferMem;
    fuse_bufvec* _buffer;

    void PrepareBuffer();
};

} // namespace FastTransport::FileCache 