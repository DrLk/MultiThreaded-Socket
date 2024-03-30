#pragma once

#include "IPacket.hpp"
#include "WriteNetworkJob.hpp"


namespace FastTransport::TaskQueue {

class ITaskScheduler;

class SendMessageJob : public WriteNetworkJob {
    using Message = Protocol::IPacket::List;

public:
    static std::unique_ptr<WriteNetworkJob> Create(Message&& message);

    SendMessageJob(Message&& message);

    void ExecuteWriteNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection) override;

private:
    Message _message;
};

} // namespace FastTransport::TaskQueue
