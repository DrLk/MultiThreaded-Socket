#include "TaskScheduler.hpp"
#include "DiskJob.hpp"
#include "ITaskScheduler.hpp"
#include "Stream.hpp"

#include "Job.hpp"
#include "MainJob.hpp"
#include "MainReadJob.hpp"
#include "WriteNetworkJob.hpp"
#include "ReadNetworkJob.hpp"

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

void TaskScheduler::ScheduleMainJob(std::unique_ptr<MainJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        _freeSendPackets = job->ExecuteMain(stop, *this, std::move(_freeSendPackets));
    });
}

void TaskScheduler::ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteMainRead(stop, *this);
    });
}

void TaskScheduler::ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteWriteNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        auto freePackets = job->ExecuteReadNetwork(stop, *this, _connection);
        _freeRecvPackets.splice(std::move(freePackets));
    });
}

void TaskScheduler::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteDisk(*this, 123.0);
    });
}

} // namespace FastTransport::TaskQueue
