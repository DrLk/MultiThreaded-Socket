#include "FuseNetworkJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void FuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<FuseNetworkJob*>(job.release());
    std::unique_ptr<FuseNetworkJob> fuseNetworkJob(pointer);
    scheduler.ScheduleFuseNetworkJob(std::move(fuseNetworkJob));
}

} // namespace FastTransport::TaskQueue
