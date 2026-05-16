#include "MessageTypeReadJob.hpp"
#include <Tracy.hpp>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "FreeRecvPacketsJob.hpp"
#include "IConnection.hpp"
#include "IStatistics.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MergeIn.hpp"
#include "MergeOut.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"
#include "ReadNetworkJob.hpp"

#define TRACER() LOGGER() << "[MessageTypeReadJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

MessageTypeReadJob::MessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages)
    : _fileTree(fileTree)
    , _messages(std::move(messages))
{
    TRACER() << "Create";
}

void MessageTypeReadJob::ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection)
{
    ZoneScopedN("MessageTypeReadJob::ExecuteReadNetwork");
    TRACER() << "Execute";

    while (_messages.empty()) {
        if (stop.stop_requested()) {
            return;
        }
        ZoneScopedN("MessageTypeReadJob::RecvFirst");
        auto messages = connection.Recv(stop, IPacket::List());
        _messages.splice(std::move(messages));
    }

    std::uint32_t messageSize = 0;
    assert(_messages.front()->GetPayload().size() >= sizeof(messageSize));
    std::memcpy(&messageSize, _messages.front()->GetPayload().data(), sizeof(messageSize));

    TRACER() << "messageSize: " << messageSize << " messageSize: " << _messages.size();
    if (messageSize > _messages.size()) {
        ZoneScopedN("MessageTypeReadJob::RecvMore");
        auto messages = connection.Recv(stop, IPacket::List());
        TRACER() << "Recv Lost: " << connection.GetStatistics().GetLostPackets()
                 << " Duplicate: " << connection.GetStatistics().GetDuplicatePackets();
        if (stop.stop_requested()) {
            return;
        }
        _messages.splice(std::move(messages));
        scheduler.Schedule(CreateNext(std::move(_messages)));
        return;
    }

    auto message = _messages.TryGenerate(messageSize);
    Protocol::MessageReader reader(std::move(message));
    MessageType type {};
    reader >> type;
    TRACER() << "MessageType: " << static_cast<int>(type);
    switch (type) {
    case MessageType::RequestTree:
        scheduler.Schedule(MergeOut::Create(_fileTree));
        break;
    case MessageType::ResponseTree:
        scheduler.Schedule(MergeIn::Create(_fileTree, std::move(reader)));
        break;
    default:
        if (!DispatchSideSpecific(type, std::move(reader), scheduler)) {
            throw std::runtime_error("MessageTypeReadJob: unknown message type for this side");
        }
        break;
    }

    scheduler.Schedule(CreateNext(std::move(_messages)));
}

} // namespace FastTransport::TaskQueue
