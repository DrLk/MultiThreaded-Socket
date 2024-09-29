#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestGetAttrJob : public FuseNetworkJob {
public:
    RequestGetAttrJob(fuse_req_t request, fuse_ino_t inode, FileSystem::RemoteFileHandle* remoteFile);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    FileSystem::RemoteFileHandle* _remoteFile;
};
} // namespace FastTransport::TaskQueue
