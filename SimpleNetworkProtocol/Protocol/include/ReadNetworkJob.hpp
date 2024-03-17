#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::Protocol {
class IConnection;
}

namespace FastTransport::TaskQueue {

using IConnection = FastTransport::Protocol::IConnection;
using IPacket = FastTransport::Protocol::IPacket;

class ReadNetworkJob : public Job {
protected:
    using IConnection = FastTransport::Protocol::IConnection;
    using Message = IPacket::List;

public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<ReadNetworkJob*>(job.release());
        std::unique_ptr<ReadNetworkJob> networkJob(pointer);
        scheduler.ScheduleReadNetworkJob(std::move(networkJob));
    }

    virtual Message ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) = 0;
};

} // namespace FastTransport::TaskQueue
