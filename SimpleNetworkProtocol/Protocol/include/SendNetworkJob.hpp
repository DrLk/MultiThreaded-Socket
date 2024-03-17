#pragma once

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "WriteNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class SendNetworkJob : public WriteNetworkJob {
    using Message = Protocol::IPacket::List;

public:
    static std::unique_ptr<SendNetworkJob> Create(Message&& message);

    SendNetworkJob(Message&& message);

    void ExecuteWriteNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection) override;

private:
    Message _message;
};

} // namespace FastTransport::TaskQueue
