#include "TaskScheduler.hpp"
#include "DiskJob.hpp"
#include "Stream.hpp"

#include "Job.hpp"
#include "NetworkJob.hpp"

namespace TaskQueue {

TaskScheduler::TaskScheduler(Stream& diskStream, Stream& networkStream)
    : _diskStream(diskStream)
    , _networkStream(networkStream)
{
}

TaskScheduler::~TaskScheduler() = default;

void TaskScheduler::Schedule(std::unique_ptr<Job>&& job)
{
    job->Accept(*this, std::move(job));
}

void TaskScheduler::Schedule(TaskType type, std::unique_ptr<Job>&& job)
{
    switch (type) {
    case TaskType::Main: {
        _mainQueue.Async([job = std::move(job), this]() mutable {
            const TaskType type = job->Execute(*this, _networkStream);
            Schedule(type, std::move(job));
        });
        break;
    }
    case TaskType::DiskWrite: {
        _diskQueue.Async([job = std::move(job), this]() mutable {
            const TaskType type = job->Execute(*this, _diskStream);
            Schedule(type, std::move(job));
        });
        break;
    }
    case TaskType::None: {
        break;
    }
    }
}

void TaskScheduler::ScheduleNetworkJob(std::unique_ptr<NetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this]() mutable {
        job->ExecuteNetwork(*this, _networkStream);
    });
}

void TaskScheduler::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this]() mutable {
        job->ExecuteDisk(*this, 123.0);
    });
}

} // namespace TaskQueue
