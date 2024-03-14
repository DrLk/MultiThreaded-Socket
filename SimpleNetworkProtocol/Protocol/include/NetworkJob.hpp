#pragma once

#include <memory>
#include <stop_token>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::Protocol {
class IConnection;
}

namespace FastTransport::TaskQueue {

using IConnection = FastTransport::Protocol::IConnection;

class NetworkJob : public Job {
public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<NetworkJob*>(job.release());
        std::unique_ptr<NetworkJob> networkJob(pointer);
        scheduler.ScheduleNetworkJob(std::move(networkJob));
    }

    virtual void ExecuteNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& stream) = 0;
};

} // namespace FastTransport::TaskQueue
