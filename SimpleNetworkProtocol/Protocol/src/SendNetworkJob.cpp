#include "SendNetworkJob.hpp"

#include "IConnection.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[SendNetworkJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

std::unique_ptr<SendNetworkJob> SendNetworkJob::Create(Message&& message)
{
    return std::make_unique<SendNetworkJob>(std::move(message));
}

SendNetworkJob::SendNetworkJob(Message&& message)
    : _message(std::move(message))
{
    TRACER() << "Create";
}

void SendNetworkJob::ExecuteWriteNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection)
{
    TRACER() << "Execute";

    connection.Send(std::move(_message));
}
} // namespace FastTransport::TaskQueue
