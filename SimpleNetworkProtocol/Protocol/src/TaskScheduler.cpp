#include "TaskScheduler.hpp"
#include <Tracy.hpp>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>

#include "CacheTreeJob.hpp"
#include "DiskJob.hpp"
#include "FileSystem/ResponseReadFileJob.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "FuseNetworkJob.hpp"
#include "ITaskScheduler.hpp"
#include "InotifyWatcherJob.hpp"
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

TaskScheduler::~TaskScheduler()
{
    TRACER() << "Stopping TaskScheduler";
    // Phase 1: signal all workers to stop simultaneously.
    // Workers that cross-schedule between queues (e.g. _readNetworkQueue
    // workers call ScheduleResponseFuseNetworkJob -> _mainQueue.Async) must
    // not be able to push into a queue whose LockedList is already destroyed.
    for (auto& diskQueue : _diskQueues) {
        diskQueue.RequestStop();
    }
    for (auto& cacheQueue : _cacheTreeQueues) {
        cacheQueue.RequestStop();
    }
    _inotifyQueue.RequestStop();
    _mainQueue.RequestStop();
    _readNetworkQueue.RequestStop();
    _writeNetworkQueue.RequestStop();

    // Phase 2: join all workers before any member destructor runs.
    // After this point no thread can push to any LockedList, so the member
    // destructors (phase 3, automatic) that clear those lists are race-free.
    _writeNetworkQueue.Join();
    _readNetworkQueue.Join();
    _mainQueue.Join();
    for (auto& diskQueue : _diskQueues) {
        diskQueue.Join();
    }
    for (auto& cacheQueue : _cacheTreeQueues) {
        cacheQueue.Join();
    }
    _inotifyQueue.Join();
}

void TaskScheduler::Schedule(std::unique_ptr<Job>&& job)
{
    job->Accept(*this, std::move(job));
}

void TaskScheduler::Wait(std::stop_token stop)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable_any stopCondition;
    stopCondition.wait(lock, stop, [] { return false; });
}

void TaskScheduler::ScheduleMainJob(std::unique_ptr<MainJob>&& job)
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

void TaskScheduler::ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleMainReadJob");
        job->ExecuteMainRead(stop, *this);
    });
}

void TaskScheduler::ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job)
{
    _writeNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleWriteNetworkJob");
        job->ExecuteWriteNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job)
{
    _readNetworkQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleReadNetworkJob");
        job->ExecuteReadNetwork(stop, *this, _connection);
    });
}

void TaskScheduler::ScheduleDiskJob(std::unique_ptr<DiskJob>&& job)
{
    const size_t idx = _nextDiskQueue.fetch_add(1, std::memory_order_relaxed) % DiskThreadCount;
    _diskQueues.at(idx).Async([job = std::move(job), this, idx](std::stop_token /*stop*/) mutable {
        ZoneScopedN("TaskScheduler::ScheduleDiskJob");
        auto freeDiskPackets = job->ExecuteDisk(*this, std::move(_freeDiskPackets.at(idx)));
        _freeDiskPackets.at(idx) = std::move(freeDiskPackets);
    });
}

void TaskScheduler::ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleFuseNetworkJob");
        if (_freeSendPackets.empty()) {
            auto freePackets = _connection.get().Send2(stop, IPacket::List());
            _freeSendPackets.splice(std::move(freePackets));
        }
        assert(!_freeSendPackets.empty());
        static constexpr size_t MaxWriterPackets = 1100;
        auto writerPackets = _freeSendPackets.TryGenerate(MaxWriterPackets);
        Protocol::MessageWriter writer(std::move(writerPackets));
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
        _freeSendPackets.splice(writer.GetPackets());
        TRACER() << "2freePackets.size(): " << _freeSendPackets.size();
    });
}

void TaskScheduler::ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseFuseNetworkJob");
        if (_freeSendPackets.size() < 2500) {
            ZoneScopedN("ScheduleResponseFuseNetworkJob::Send2Wait");
            auto freePackets = _connection.get().Send2(stop, IPacket::List());
            _freeSendPackets.splice(std::move(freePackets));
        }
        assert(!_freeSendPackets.empty());
        static constexpr size_t MaxWriterPackets = 1100;
        auto writerPackets = _freeSendPackets.TryGenerate(MaxWriterPackets);
        Protocol::MessageWriter writer(std::move(writerPackets));
        Message freePackets = [&] {
            ZoneScopedN("ScheduleResponseFuseNetworkJob::ExecuteResponse");
            return job->ExecuteResponse(stop, writer, _fileTree);
        }();
        freePackets.splice(job->GetFreeReadPackets());

        auto writedPackets = writer.GetWritedPackets();
        TRACER() << "writedPackets.size(): " << writedPackets.size();
        if (!writedPackets.empty()) {
            ScheduleWriteNetworkJob(std::make_unique<SendMessageJob>(std::move(writedPackets)));
        }

        if (!freePackets.empty()) {
            ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
        }
        _freeSendPackets.splice(writer.GetPackets());
        TRACER() << "1freePackets.size(): " << _freeSendPackets.size();
    });
}

void TaskScheduler::ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job)
{
    // Phase 1 (mainQueue): allocate send packets — _freeSendPackets is mainQueue-only.
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseReadDiskJob::alloc");
        {
            auto freePackets = _connection.get().TryGetFreeSendPackets();
            _freeSendPackets.splice(std::move(freePackets));
        }
        static constexpr size_t MaxWriterPackets = 1100;
        // Ensure we have at least MaxWriterPackets so GetDataPackets(1000) always
        // receives 1000 packets (headers consume 1 packet, leaving 1099 for data).
        while (_freeSendPackets.size() < MaxWriterPackets) {
            ZoneScopedN("ScheduleResponseReadDiskJob::Send2Wait");
            auto freePackets = _connection.get().Send2(stop, IPacket::List());
            _freeSendPackets.splice(std::move(freePackets));
        }
        assert(!_freeSendPackets.empty());
        auto writerPackets = _freeSendPackets.TryGenerate(MaxWriterPackets);

        // Phase 2 (disk thread): perform the blocking file read.
        const size_t idx = _nextDiskQueue.fetch_add(1, std::memory_order_relaxed) % DiskThreadCount;
        _diskQueues.at(idx).Async(
            [job = std::move(job), writerPackets = std::move(writerPackets), this](std::stop_token diskStop) mutable {
                ZoneScopedN("TaskScheduler::ScheduleResponseReadDiskJob::read");
                Protocol::MessageWriter writer(std::move(writerPackets));
                Message unusedDataPackets = job->ExecuteResponse(diskStop, writer, _fileTree);
                Message freeRecvPackets = job->GetFreeReadPackets();

                auto writedPackets = writer.GetWritedPackets();
                TRACER() << "ScheduleResponseReadDiskJob writedPackets.size(): " << writedPackets.size();
                if (!writedPackets.empty()) {
                    ScheduleWriteNetworkJob(std::make_unique<SendMessageJob>(std::move(writedPackets)));
                }
                if (!freeRecvPackets.empty()) {
                    ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freeRecvPackets)));
                }
                // Return unused write packets + unused data packets back to mainQueue.
                auto unusedPackets = writer.GetPackets();
                unusedPackets.splice(std::move(unusedDataPackets));
                if (!unusedPackets.empty()) {
                    _mainQueue.Async([unusedPackets = std::move(unusedPackets), this](std::stop_token /*s*/) mutable {
                        for (auto& packet : unusedPackets) {
                            packet->SetPayloadSize(Protocol::MaxPayloadSize);
                        }
                        _freeSendPackets.splice(std::move(unusedPackets));
                    });
                }
            });
    });
}

void TaskScheduler::ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job)
{
    _mainQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseInFuseNetworkJob");
        assert(!_freeSendPackets.empty());
        Message freePackets = job->ExecuteResponse(*this, stop, _fileTree);
        freePackets.splice(job->GetFreeReadPackets());

        if (!freePackets.empty()) {
            ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
        }
    });
}

void TaskScheduler::ScheduleCacheTreeJob(std::unique_ptr<CacheTreeJob>&& job)
{
    const size_t idx = std::hash<fuse_ino_t> {}(job->GetInode()) % CacheTreeThreadCount;
    _cacheTreeQueues.at(idx).Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleCacheTreeJob");
        job->ExecuteCachedTree(*this, stop, _fileTree);
    });
}

void TaskScheduler::ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job)
{
    _inotifyQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleInotifyWatcherJob");
        job->ExecuteMainRead(stop, *this);
    });
}

void TaskScheduler::ReturnFreeDiskPackets(Protocol::IPacket::List&& packets)
{
    const size_t idx = _nextDiskQueue.fetch_add(1, std::memory_order_relaxed) % DiskThreadCount;
    _diskQueues.at(idx).Async([packets = std::move(packets), this, idx](std::stop_token /*stop*/) mutable {
        _freeDiskPackets.at(idx).splice(std::move(packets));
    });
}

} // namespace FastTransport::TaskQueue
