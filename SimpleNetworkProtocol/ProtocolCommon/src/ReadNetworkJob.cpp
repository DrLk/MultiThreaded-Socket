#include "ReadNetworkJob.hpp"

#include <memory>
#include <utility>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void ReadNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<ReadNetworkJob*>(job.release());
    std::unique_ptr<ReadNetworkJob> networkJob(pointer);
    scheduler.ScheduleReadNetworkJob(std::move(networkJob));
}

} // namespace FastTransport::TaskQueue
