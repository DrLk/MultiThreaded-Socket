#include "MergeRequest.hpp"

#include <memory>
#include <stop_token>

#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MainJob.hpp"
#include "MessageType.hpp"
#include "MessageWriter.hpp"
#include "SendMessageJob.hpp"

#define TRACER() LOGGER() << "[MergeRequest] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

std::unique_ptr<MainJob> MergeRequest::Create()
{
    return std::make_unique<MergeRequest>();
}

MergeRequest::MergeRequest()
{
    TRACER() << "Create";
}

MergeRequest::Message MergeRequest::ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& message)
{
    TRACER() << "Execute";

    Protocol::MessageWriter writer(std::move(message));
    writer << MessageType::RequestTree;

    scheduler.Schedule(SendMessageJob::Create(writer.GetWritedPackets()));
    return writer.GetPackets();
}

} // namespace FastTransport::TaskQueue
