#include "FuseNetworkJob.hpp"

#include <memory>

#include "IClientTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void FuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    // Accept is virtually dispatched on the runtime type of *job, so the
    // static type is guaranteed to be FuseNetworkJob: a static_cast cannot
    // lose the pointer the way `dynamic_cast<T*>(release())` could on a
    // null-cast failure.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    std::unique_ptr<FuseNetworkJob> fuseNetworkJob(static_cast<FuseNetworkJob*>(job.release()));
    dynamic_cast<IClientTaskScheduler&>(scheduler).ScheduleFuseNetworkJob(std::move(fuseNetworkJob));
}

void FuseNetworkJob::InitReader(Reader&& reader)
{
    _reader = std::move(reader);
}

FuseNetworkJob::Reader& FuseNetworkJob::GetReader()
{
    return _reader;
}

FuseNetworkJob::Message FuseNetworkJob::GetFreeReadPackets()
{
    return _reader.GetPackets();
}

} // namespace FastTransport::TaskQueue
