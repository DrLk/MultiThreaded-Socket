#include "TaskScheduler.hpp"
#include <Tracy.hpp>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>

#include "DiskJob.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Logger.hpp"
#include "MainJob.hpp"
#include "MainReadJob.hpp"
#include "ReadNetworkJob.hpp"
#include "SendMessageJob.hpp"
#include "WriteNetworkJob.hpp"

#define TRACER() LOGGER() << "[TaskScheduler] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

// ===========================================================================
// Common implementation: ctor/dtor, generic Schedule, Wait, and the
// Schedule methods that exist on both server and client sides.
// ===========================================================================

TaskSchedulerBase::TaskSchedulerBase(IConnection& connection, FileTree& fileTree)
    : _connection(connection)
    , _fileTree(fileTree)
{
}

TaskSchedulerBase::~TaskSchedulerBase() // NOLINT(bugprone-exception-escape)
{
    // Defensive: derived destructors are required to call ShutdownCommonQueues
    // before returning so worker threads cannot read `*this`'s vptr while it is
    // transitioning from the derived to the base class. Calling it again here
    // is a no-op (TaskQueue::Join on an already-joined queue is a no-op).
    ShutdownCommonQueues();
}

void TaskSchedulerBase::ShutdownCommonQueues() noexcept
{
    TRACER() << "Stopping TaskScheduler";
    // Phase 1: signal all common workers to stop.
    for (auto& diskQueue : _diskQueues) {
        diskQueue.RequestStop();
    }
    _mainQueue.RequestStop();
    _readNetworkQueue.RequestStop();
    _writeNetworkQueue.RequestStop();

    // Phase 2: join all common workers.
    _writeNetworkQueue.Join();
    _readNetworkQueue.Join();
    _mainQueue.Join();
    for (auto& diskQueue : _diskQueues) {
        diskQueue.Join();
    }
}

void TaskSchedulerBase::Schedule(std::unique_ptr<Job>&& job)
{
    job->Accept(*this, std::move(job));
}

void TaskSchedulerBase::Wait(std::stop_token stop)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable_any stopCondition;
    stopCondition.wait(lock, stop, [] { return false; });
}

void TaskSchedulerBase::ScheduleMainJob(std::unique_ptr<MainJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleMainJob");
        auto freePackets = _connection.get().Send2(stop, IPacket::List());
        _freeSendPackets.splice(std::move(freePackets));

        TRACER() << "3freePackets.size(): " << _freeSendPackets.size();
        _freeSendPackets = job->ExecuteMain(stop, *this, std::move(_freeSendPackets));
        assert(!_freeSendPackets.empty());
    });
}

void TaskSchedulerBase::ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleMainReadJob");
        job->ExecuteMainRead(stop, *this);
    });
}

void TaskSchedulerBase::ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job)
{
    _writeNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleWriteNetworkJob");
        job->ExecuteWriteNetwork(stop, *this, _connection);
    });
}

void TaskSchedulerBase::ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job)
{
    _readNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleReadNetworkJob");
        job->ExecuteReadNetwork(stop, *this, _connection);
    });
}

void TaskSchedulerBase::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    const size_t idx = _nextDiskQueue.fetch_add(1, std::memory_order_relaxed) % DiskThreadCount;
    _diskQueues.at(idx).Async([job = std::move(job), this, idx](std::stop_token /*stop*/) mutable {
        ZoneScopedN("TaskScheduler::ScheduleDiskJob");
        auto freeDiskPackets = job->ExecuteDisk(*this, std::move(_freeDiskPackets.at(idx)));
        _freeDiskPackets.at(idx) = std::move(freeDiskPackets);
    });
}

void TaskSchedulerBase::ReturnFreeDiskPackets(Protocol::IPacket::List&& packets)
{
    const size_t idx = _nextDiskQueue.fetch_add(1, std::memory_order_relaxed) % DiskThreadCount;
    _diskQueues.at(idx).Async([packets = std::move(packets), this, idx](std::stop_token /*stop*/) mutable {
        _freeDiskPackets.at(idx).splice(std::move(packets));
    });
}

} // namespace FastTransport::TaskQueue
