#include "DiskJob.hpp"

#include <memory>
#include <utility>

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

void DiskJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<DiskJob*>(job.release());
    std::unique_ptr<DiskJob> diskJob(pointer);
    scheduler.ScheduleDiskJob(std::move(diskJob));
}

} // namespace FastTransport::TaskQueue
