#pragma once

#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {
    using Writer = Protocol::MessageWriter;

    class RequestReadFileJob : public FuseNetworkJob {
        public:
            RequestReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo);
            Message ExecuteMain(std::stop_token stop, Writer& writer) override;

        private:
            fuse_req_t _request;
            fuse_ino_t _inode;
            size_t _size;
            off_t _offset;
            fuse_file_info* _fileInfo;
    };
} // namespace FastTransport::TaskQueue
