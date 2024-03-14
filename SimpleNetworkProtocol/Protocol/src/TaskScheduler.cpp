#include "TaskScheduler.hpp"
#include "DiskJob.hpp"
#include "Stream.hpp"

#include "Job.hpp"
#include "NetworkJob.hpp"

namespace FastTransport::TaskQueue {

TaskScheduler::TaskScheduler(Stream& diskStream, IConnection& connection)
    : _diskStream(diskStream)
    , _connection(connection)
{
}

TaskScheduler::~TaskScheduler() = default;

void TaskScheduler::Schedule(std::unique_ptr<Job>&& job)
{
    job->Accept(*this, std::move(job));
}

void TaskScheduler::ScheduleNetworkJob(std::unique_ptr<NetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteDisk(*this, 123.0);
    });
}

} // namespace FastTransport::TaskQueue
