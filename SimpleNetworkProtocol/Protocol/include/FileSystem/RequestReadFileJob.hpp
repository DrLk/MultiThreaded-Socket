#pragma once

#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "MainJob.hpp"
#include "FuseNetworkJob.hpp"

namespace FastTransport::FileSystem {
    class File;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {
    using File = FileSystem::File;
    using Writer = Protocol::MessageWriter;

    class RequestReadFileJob : public FuseNetworkJob {
        public:
            RequestReadFileJob(File& file, fuse_req_t request, size_t size, off_t off);
            Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Writer& writer) override;
            
        private:
            File& _file;
            fuse_req_t _request;
            size_t _size;
            off_t _off;
    };
} // namespace FastTransport::TaskQueue
