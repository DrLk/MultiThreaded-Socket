#pragma once

#include "IPacket.hpp"
#include "Job.hpp"
#include <stop_token>

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class MainReadJob : public Job {
protected:
    using Message = Protocol::IPacket::List;

public:
    virtual void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) = 0;

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;
};

} // namespace FastTransport::TaskQueue
