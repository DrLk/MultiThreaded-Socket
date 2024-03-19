#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "Job.hpp"

namespace FastTransport::Protocol {
class IConnection;
}

namespace FastTransport::TaskQueue {

class ITaskScheduler;

using IConnection = FastTransport::Protocol::IConnection;
using IPacket = FastTransport::Protocol::IPacket;

class ReadNetworkJob : public Job {
protected:
    using IConnection = FastTransport::Protocol::IConnection;
    using Message = IPacket::List;

public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    virtual void ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) = 0;
};

} // namespace FastTransport::TaskQueue
