#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestReleaseJob : public FuseNetworkJob {
public:
    RequestReleaseJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    FileHandle* _handle;
};
} // namespace FastTransport::TaskQueue
