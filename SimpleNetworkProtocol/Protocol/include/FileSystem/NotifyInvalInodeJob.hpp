#pragma once

#include "ResponseInFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class NotifyInvalInodeJob : public ResponseInFuseNetworkJob {
public:
    Message ExecuteResponse(ITaskScheduler& scheduler, std::stop_token stop, FileTree& fileTree) override;
};

} // namespace FastTransport::TaskQueue
