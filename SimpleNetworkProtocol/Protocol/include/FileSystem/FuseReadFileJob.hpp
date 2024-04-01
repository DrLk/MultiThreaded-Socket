#pragma once

#include <cstddef>
#include <fuse3/fuse_lowlevel.h>

#include "DiskJob.hpp"
#include "MainJob.hpp"

namespace FastTransport::FileSystem {
    class File;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {
    using File = FileSystem::File;
    using Message = Protocol::IPacket::List;

    class FuseReadFileJob : public MainJob {
        public:
            FuseReadFileJob(File& file, fuse_req_t request, size_t size, off_t off);
            [[nodiscard]] Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& freePackets) override;
            
        private:
            File& _file;
            fuse_req_t _request;
            size_t _size;
            off_t _off;
    };
} // namespace FastTransport::TaskQueue
