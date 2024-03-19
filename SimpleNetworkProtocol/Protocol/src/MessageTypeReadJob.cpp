#include "MessageTypeReadJob.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "FreeRecvPacketsJob.hpp"
#include "IConnection.hpp"
#include "ITaskScheduler.hpp"
#include "MergeIn.hpp"
#include "MergeOut.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"

namespace FastTransport::TaskQueue {
std::unique_ptr<MessageTypeReadJob> MessageTypeReadJob::Create(FileTree& fileTree, Message&& messages)
{
    return std::make_unique<MessageTypeReadJob>(fileTree, std::move(messages));
}

MessageTypeReadJob::MessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages)
    : _fileTree(fileTree)
    , _messages(std::move(messages))
{
    TRACER() << "Create";
}

void MessageTypeReadJob::ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, Protocol::IConnection& connection)
{
    TRACER() << "Execute";

    auto messages = connection.Recv(stop, IPacket::List());
    _messages.splice(std::move(messages));

    if (_messages.empty()) {
        scheduler.Schedule(MessageTypeReadJob::Create(_fileTree, std::move(_messages)));
        return;
    }

    std::uint32_t messageSize;
    assert(_messages.front()->GetPayload().size() >= sizeof(messageSize));
    std::memcpy(&messageSize, _messages.front()->GetPayload().data(), sizeof(messageSize));

    if (messageSize < _messages.size()) {
        scheduler.Schedule(MessageTypeReadJob::Create(_fileTree, std::move(_messages)));
        return;
    }

    auto message = _messages.TryGenerate(messageSize);
    Protocol::MessageReader reader(std::move(message));
    MessageType type;
    reader >> type;
    switch (type) {
    case MessageType::RequestTree:
        scheduler.Schedule(MergeOut::Create(_fileTree));
        scheduler.Schedule(FreeRecvPacketsJob::Create(reader.GetPackets()));
        break;
    case MessageType::ResponseTree:
        scheduler.Schedule(MergeIn::Create(_fileTree, std::move(reader)));
        break;
    case MessageType::RequestFile:
        break;
    case MessageType::ResponseFile:
        break;
    default:
        throw std::runtime_error("MessageTypeReadJob: unknown message type");
    }

    scheduler.Schedule(MessageTypeReadJob::Create(_fileTree, std::move(_messages)));
}

} // namespace FastTransport::TaskQueue
