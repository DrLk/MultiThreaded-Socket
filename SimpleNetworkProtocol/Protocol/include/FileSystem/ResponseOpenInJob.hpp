#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "MessageReader.hpp"
#include "ResponseInFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class ResponseOpenInJob : public ResponseInFuseNetworkJob {
    using Reader = Protocol::MessageReader;
    using FileTree = FileSystem::FileTree;

public:
    Message ExecuteResponse(ITaskScheduler& scheduler, std::stop_token stop, FileTree& fileTree) override;
};
} // namespace FastTransport::TaskQueue
