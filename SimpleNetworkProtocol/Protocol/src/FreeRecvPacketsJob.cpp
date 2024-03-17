#include "FreeRecvPacketsJob.hpp"

#include <memory>

#include "IConnection.hpp"
#include "IPacket.hpp"

namespace FastTransport::TaskQueue {

std::unique_ptr<FreeRecvPacketsJob> FreeRecvPacketsJob::Create(Message&& message)
{
    return std::make_unique<FreeRecvPacketsJob>(std::move(message));
}

FreeRecvPacketsJob::FreeRecvPacketsJob(Message&& message)
    : _message(std::move(message))
{
}

void FreeRecvPacketsJob::ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection)
{
    connection.AddFreeRecvPackets(std::move(_message));
}

} // namespace FastTransport::TaskQueue
