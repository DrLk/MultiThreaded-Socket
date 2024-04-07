#include "MessageTypeReadJob.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "FreeRecvPacketsJob.hpp"
#include "IConnection.hpp"
#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MergeIn.hpp"
#include "MergeOut.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"
#include "ReadNetworkJob.hpp"
#include "ResponseForgetMultiInJob.hpp"
#include "ResponseForgetMultiJob.hpp"
#include "ResponseGetAttrJob.hpp"
#include "ResponseGetAttrJobIn.hpp"
#include "ResponseLookupJob.hpp"
#include "ResponseLookupJobIn.hpp"
#include "ResponseOpenDirInJob.hpp"
#include "ResponseOpenDirJob.hpp"

#define TRACER() LOGGER() << "[MessageTypeReadJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {
std::unique_ptr<ReadNetworkJob> MessageTypeReadJob::Create(FileTree& fileTree, Message&& messages)
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

    if (_messages.empty()) {
        auto messages = connection.Recv(stop, IPacket::List());
        _messages.splice(std::move(messages));
    }

    std::uint32_t messageSize = 0;
    assert(_messages.front()->GetPayload().size() >= sizeof(messageSize));
    std::memcpy(&messageSize, _messages.front()->GetPayload().data(), sizeof(messageSize));

    TRACER() << "messageSize: " << messageSize;
    if (messageSize > _messages.size()) {
        auto messages = connection.Recv(stop, IPacket::List());
        _messages.splice(std::move(messages));
        scheduler.Schedule(MessageTypeReadJob::Create(_fileTree, std::move(_messages)));
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
    case MessageType::RequestGetAttr: {
        auto job = std::make_unique<ResponseGetAttrJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::ResponseGetAttr: {
        auto job = std::make_unique<ResponseGetAttrJobIn>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::RequestLookup: {
        auto job = std::make_unique<ResponseLookupJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::ResponseLookup: {
        auto job = std::make_unique<ResponseLookupJobIn>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::RequestOpenDir: {
        auto job = std::make_unique<ResponseOpenDirJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::ResponseOpenDir: {
        auto job = std::make_unique<ResponseOpenDirInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::RequestForgetMulti: {
        auto job = std::make_unique<ResponseForgetMultiJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    case MessageType::ResponseForgetMulti: {
        auto job = std::make_unique<ResponseForgetMultiInJob>();
        job->InitReader(std::move(reader));
        scheduler.Schedule(std::move(job));
        break;
    }
    default:
        throw std::runtime_error("MessageTypeReadJob: unknown message type");
    }

    scheduler.Schedule(MessageTypeReadJob::Create(_fileTree, std::move(_messages)));
}

} // namespace FastTransport::TaskQueue
