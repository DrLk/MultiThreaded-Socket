#include "ServerTaskScheduler.hpp"
#include <Tracy.hpp>

#include <memory>
#include <stop_token>

#include "FileSystem/ResponseReadFileJob.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "InotifyWatcherJob.hpp"
#include "Logger.hpp"
#include "ResponseFuseNetworkJob.hpp"
#include "SendMessageJob.hpp"

#define TRACER() LOGGER() << "[TaskScheduler] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

// ===========================================================================
// Server-only implementation: handlers for incoming requests, outgoing
// responses, server-side disk reads, and inotify watching.
// ===========================================================================

ServerTaskScheduler::ServerTaskScheduler(IConnection& connection, FileTree& fileTree)
    : TaskSchedulerBase(connection, fileTree)
{
}

ServerTaskScheduler::~ServerTaskScheduler() // NOLINT(bugprone-exception-escape)
{
    // Stop and join the inotify queue first — its worker may schedule onto
    // _mainQueue. Then drain the common queues BEFORE returning, so the vptr
    // transition into ~TaskSchedulerBase happens with no live workers that
    // could observe a half-changed vtable via virtual dispatch on `*this`.
    _inotifyQueue.RequestStop();
    _inotifyQueue.Join();
    ShutdownCommonQueues();
}

void ServerTaskScheduler::ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job)
{
    PostToMainQueue([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseFuseNetworkJob");
        if (FreeSendPackets().size() < 2500) {
            ZoneScopedN("ScheduleResponseFuseNetworkJob::Send2Wait");
            auto freePackets = Connection().Send2(stop, IPacket::List());
            FreeSendPackets().splice(std::move(freePackets));
        }
        assert(!FreeSendPackets().empty());
        static constexpr size_t MaxWriterPackets = 1100;
        auto writerPackets = FreeSendPackets().TryGenerate(MaxWriterPackets);
        Protocol::MessageWriter writer(std::move(writerPackets));
        Message freePackets = [&] {
            ZoneScopedN("ScheduleResponseFuseNetworkJob::ExecuteResponse");
            return job->ExecuteResponse(stop, writer, Tree());
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
        FreeSendPackets().splice(writer.GetPackets());
        TRACER() << "1freePackets.size(): " << FreeSendPackets().size();
    });
}

void ServerTaskScheduler::ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job)
{
    // Phase 1 (mainQueue): allocate send packets — _freeSendPackets is mainQueue-only.
    PostToMainQueue([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseReadDiskJob::alloc");
        {
            auto freePackets = Connection().TryGetFreeSendPackets();
            FreeSendPackets().splice(std::move(freePackets));
        }
        static constexpr size_t MaxWriterPackets = 1100;
        // Ensure we have at least MaxWriterPackets so GetDataPackets(1000) always
        // receives 1000 packets (headers consume 1 packet, leaving 1099 for data).
        while (FreeSendPackets().size() < MaxWriterPackets) {
            ZoneScopedN("ScheduleResponseReadDiskJob::Send2Wait");
            auto freePackets = Connection().Send2(stop, IPacket::List());
            FreeSendPackets().splice(std::move(freePackets));
        }
        assert(!FreeSendPackets().empty());
        auto writerPackets = FreeSendPackets().TryGenerate(MaxWriterPackets);

        // Phase 2 (disk thread): perform the blocking file read.
        PostToDiskQueue(
            [job = std::move(job), writerPackets = std::move(writerPackets), this](std::stop_token diskStop) mutable {
                ZoneScopedN("TaskScheduler::ScheduleResponseReadDiskJob::read");
                Protocol::MessageWriter writer(std::move(writerPackets));
                Message unusedDataPackets = job->ExecuteResponse(diskStop, writer, Tree());
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
                    PostToMainQueue([unusedPackets = std::move(unusedPackets), this](std::stop_token /*s*/) mutable {
                        for (auto& packet : unusedPackets) {
                            packet->SetPayloadSize(Protocol::MaxPayloadSize);
                        }
                        FreeSendPackets().splice(std::move(unusedPackets));
                    });
                }
            });
    });
}

void ServerTaskScheduler::ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job)
{
    _inotifyQueue.Async([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleInotifyWatcherJob");
        job->ExecuteMainRead(stop, *this);
    });
}

} // namespace FastTransport::TaskQueue
