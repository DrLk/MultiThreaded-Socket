#include "WriteNetworkJob.hpp"

#include <memory>
#include <utility>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void WriteNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<WriteNetworkJob*>(job.release());
    std::unique_ptr<WriteNetworkJob> networkJob(pointer);
    scheduler.ScheduleWriteNetworkJob(std::move(networkJob));
}

} // namespace FastTransport::TaskQueue
