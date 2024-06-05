#include "CacheTreeJob.hpp"

#include "ITaskScheduler.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[ReadFileCacheJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

void CacheTreeJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job)
{
    auto* pointer = dynamic_cast<CacheTreeJob*>(job.release());
    std::unique_ptr<CacheTreeJob> cacheTreeJob(pointer);
    scheduler.ScheduleCacheTreeJob(std::move(cacheTreeJob));
}

} // namespace FastTransport::TaskQueue
