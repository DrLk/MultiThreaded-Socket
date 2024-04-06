#pragma once

#include <cstddef>
#include <functional>
#include <fuse3/fuse_lowlevel.h>

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
            Message ExecuteMain(std::stop_token stop, Writer& writer) override;

        private:
            std::reference_wrapper<File> _file;
            fuse_req_t _request;
            size_t _size;
            off_t _off;
    };
} // namespace FastTransport::TaskQueue
