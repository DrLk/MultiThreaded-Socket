#include "ClientTaskScheduler.hpp"
#include <Tracy.hpp>

#include <memory>
#include <stop_token>

#include "FreeRecvPacketsJob.hpp"
#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "ResponseInFuseNetworkJob.hpp"
#include "SendMessageJob.hpp"

#define TRACER() LOGGER() << "[TaskScheduler] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

// ===========================================================================
// Client-only implementation: handlers for outgoing requests, incoming
// responses, and FUSE-side cache tree work.
// ===========================================================================

ClientTaskScheduler::ClientTaskScheduler(IConnection& connection, FileTree& fileTree)
    : TaskSchedulerBase(connection, fileTree)
{
}

ClientTaskScheduler::~ClientTaskScheduler() // NOLINT(bugprone-exception-escape)
{
    // Drain the common queues BEFORE returning so the vptr transition into
    // ~TaskSchedulerBase happens with no live workers that could observe a
    // half-changed vtable via virtual dispatch on `*this`.
    ShutdownCommonQueues();
}

void ClientTaskScheduler::ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job)
{
    PostToMainQueue([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleFuseNetworkJob");
        if (FreeSendPackets().empty()) {
            auto freePackets = Connection().Send2(stop, IPacket::List());
            FreeSendPackets().splice(std::move(freePackets));
        }
        assert(!FreeSendPackets().empty());
        static constexpr size_t MaxWriterPackets = 1100;
        auto writerPackets = FreeSendPackets().TryGenerate(MaxWriterPackets);
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
        FreeSendPackets().splice(writer.GetPackets());
        TRACER() << "2freePackets.size(): " << FreeSendPackets().size();
    });
}

void ClientTaskScheduler::ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job)
{
    PostToMainQueue([job = std::move(job), this](std::stop_token stop) mutable {
        ZoneScopedN("TaskScheduler::ScheduleResponseInFuseNetworkJob");
        assert(!FreeSendPackets().empty());
        Message freePackets = job->ExecuteResponse(*this, stop, Tree());
        freePackets.splice(job->GetFreeReadPackets());

        if (!freePackets.empty()) {
            ScheduleReadNetworkJob(std::make_unique<FreeRecvPacketsJob>(std::move(freePackets)));
        }
    });
}

} // namespace FastTransport::TaskQueue
