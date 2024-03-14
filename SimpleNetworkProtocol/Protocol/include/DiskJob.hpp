#pragma once

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

class DiskJob : public Job {
public:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    {
        auto* pointer = dynamic_cast<DiskJob*>(job.release());
        std::unique_ptr<DiskJob> diskJob(pointer);
        scheduler.ScheduleDiskJob(std::move(diskJob));
    }

    virtual void ExecuteDisk(ITaskScheduler& scheduler, float disk) = 0;
};

} // namespace FastTransport::TaskQueue
