#include "TaskScheduler.hpp"

#include <chrono>
#include <memory>
#include <stop_token>
#include <thread>

#include "DiskJob.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "MainJob.hpp"
#include "MainReadJob.hpp"
#include "ReadNetworkJob.hpp"
#include "WriteNetworkJob.hpp"

namespace FastTransport::TaskQueue {

TaskScheduler::TaskScheduler(IConnection& connection)
    : _connection(connection)
{
}

TaskScheduler::~TaskScheduler() = default;

void TaskScheduler::Schedule(std::unique_ptr<Job>&& job)
{
    job->Accept(*this, std::move(job));
}

void TaskScheduler::Wait(std::stop_token stop)
{
    while (!stop.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void TaskScheduler::ScheduleMainJob(std::unique_ptr<MainJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        auto freePackets = _connection.get().Send2(stop, IPacket::List());
        _freeSendPackets.splice(std::move(freePackets));
        assert(!_freeSendPackets.empty());
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
    _writeNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteWriteNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job)
{
    _readNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteReadNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        job->ExecuteDisk(*this, 123.0);
    });
}

} // namespace FastTransport::TaskQueue
