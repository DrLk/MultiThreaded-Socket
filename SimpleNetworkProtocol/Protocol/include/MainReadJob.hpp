#pragma once

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include <stop_token>

namespace FastTransport::TaskQueue {

class MainReadJob : public Job {
protected:
    using Message = Protocol::IPacket::List;

public:
    virtual void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) = 0;

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<MainReadJob*>(job.release());
        std::unique_ptr<MainReadJob> mainJob(pointer);
        scheduler.ScheduleMainReadJob(std::move(mainJob));
    }
};

} // namespace FastTransport::TaskQueue
