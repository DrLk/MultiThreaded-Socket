#include "MainReadJob.hpp"

#include <memory>
#include <utility>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void MainReadJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<MainReadJob*>(job.release());
    std::unique_ptr<MainReadJob> mainJob(pointer);
    scheduler.ScheduleMainReadJob(std::move(mainJob));
}

} // namespace FastTransport::TaskQueue
