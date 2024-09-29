#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
using Writer = Protocol::MessageWriter;

class RequestReadDirJob : public FuseNetworkJob {
public:
    RequestReadDirJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t off, FileSystem::RemoteFileHandle* remoteFile);
    Message ExecuteMain(std::stop_token stop, Writer& writer) override;

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _off;
    FileSystem::RemoteFileHandle* _remoteFile;
};
} // namespace FastTransport::TaskQueue
