#pragma once

#include <memory>

#include "ITaskScheduler.hpp"
#include "ReadNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class FreeRecvPacketsJob : public ReadNetworkJob {
public:
    static std::unique_ptr<FreeRecvPacketsJob> Create(Message&& message);

    FreeRecvPacketsJob(Message&& message);

    void ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) override;

private:
    Message _message;
};

} // namespace FastTransport::TaskQueue
