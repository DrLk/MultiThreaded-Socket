#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestOpenDirJob : public FuseNetworkJob {
public:
    RequestOpenDirJob(fuse_req_t request, fuse_ino_t inode, struct fuse_file_info* fileInfo);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    fuse_file_info* _fileInfo;
};
} // namespace FastTransport::TaskQueue
