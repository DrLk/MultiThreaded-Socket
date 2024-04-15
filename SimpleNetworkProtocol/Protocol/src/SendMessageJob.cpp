#include "SendMessageJob.hpp"

#include "IConnection.hpp"
#include "Logger.hpp"
#include "WriteNetworkJob.hpp"

#define TRACER() LOGGER() << "[SendNetworkJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

std::unique_ptr<WriteNetworkJob> SendMessageJob::Create(Message&& message)
{
    return std::make_unique<SendMessageJob>(std::move(message));
}

SendMessageJob::SendMessageJob(Message&& message)
    : _message(std::move(message))
{
    TRACER() << "Create";
}

void SendMessageJob::ExecuteWriteNetwork(std::stop_token  /*stop*/, ITaskScheduler&  /*scheduler*/, Protocol::IConnection& connection)
{
    TRACER() << "Execute";

    connection.Send(std::move(_message));
}
} // namespace FastTransport::TaskQueue
