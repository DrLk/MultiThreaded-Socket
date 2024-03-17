#pragma once

#include <memory>
#include <stop_token>

#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

class MainJob : public Job {
protected:
    using Message = Protocol::IPacket::List;

public:
    [[nodiscard]] virtual Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& freePackets) = 0;

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<MainJob*>(job.release());
        std::unique_ptr<MainJob> mainJob(pointer);
        scheduler.ScheduleMainJob(std::move(mainJob));
    }
};

} // namespace FastTransport::TaskQueue
