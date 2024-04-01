#pragma once

#include "Job.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class DiskJob : public Job {
public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    virtual void ExecuteDisk(ITaskScheduler& scheduler, float disk) = 0;
};

} // namespace FastTransport::TaskQueue
