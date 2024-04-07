#include "TaskScheduler.hpp"

#include <chrono>
#include <memory>
#include <stop_token>
#include <thread>

#include "DiskJob.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "FuseNetworkJob.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Logger.hpp"
#include "MainJob.hpp"
#include "MainReadJob.hpp"
#include "ReadNetworkJob.hpp"
#include "ResponseFuseNetworkJob.hpp"
#include "SendMessageJob.hpp"
#include "WriteNetworkJob.hpp"


#define TRACER() LOGGER() << "[TaskScheduler] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

TaskScheduler::TaskScheduler(IConnection& connection, FileTree& fileTree)
    : _connection(connection)
    , _fileTree(fileTree)
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
        _freeSendPackets = job->ExecuteMain(stop, *this, std::move(_freeSendPackets));
        assert(!_freeSendPackets.empty());
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
    _mainQueue.Async([job = std::move(job), this](std::stop_token /*stop*/) mutable {
        job->ExecuteDisk(*this, 123.0);
    });
}

void TaskScheduler::ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        assert(!_freeSendPackets.empty());
        Protocol::MessageWriter writer(std::move(_freeSendPackets));
        Message freePackets = job->ExecuteMain(stop, writer);
        freePackets.splice(job->GetFreeReadPackets());

        auto writedPackets = writer.GetWritedPackets();
        TRACER() << "writedPackets.size() 2: " << writedPackets.size();
        if (!writedPackets.empty()) {
            ScheduleWriteNetworkJob(std::make_unique<SendMessageJob>(std::move(writedPackets)));
        }

        if (!freePackets.empty()) {
            ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
        }
        _freeSendPackets = writer.GetPackets();
    });
}

void TaskScheduler::ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        assert(!_freeSendPackets.empty());
        Protocol::MessageWriter writer(std::move(_freeSendPackets));
        Message freePackets = job->ExecuteResponse(stop, writer, _fileTree);
        freePackets.splice(job->GetFreeReadPackets());

        auto writedPackets = writer.GetWritedPackets();
        TRACER() << "writedPackets.size(): " << writedPackets.size();
        if (!writedPackets.empty()) {
            ScheduleWriteNetworkJob(std::make_unique<SendMessageJob>(std::move(writedPackets)));
        }

        if (!freePackets.empty()) {
            ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
        }
        _freeSendPackets = writer.GetPackets();
    });
}

} // namespace FastTransport::TaskQueue
