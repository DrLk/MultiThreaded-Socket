#pragma once

#include <memory>

#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Stream.hpp"

namespace TaskQueue {

class NetworkJob : public Job {
public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<NetworkJob*>(job.release());
        std::unique_ptr<NetworkJob> networkJob(pointer);
        scheduler.ScheduleNetworkJob(std::move(networkJob));
    }

    virtual void ExecuteNetwork(ITaskScheduler& scheduler, Stream& stream) = 0;
};

} // namespace TaskQueue
