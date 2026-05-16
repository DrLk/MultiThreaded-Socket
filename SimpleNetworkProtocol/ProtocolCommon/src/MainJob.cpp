#include "MainJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void MainJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<MainJob*>(job.release());
    std::unique_ptr<MainJob> mainJob(pointer);
    scheduler.ScheduleMainJob(std::move(mainJob));
}

} // namespace FastTransport::TaskQueue
