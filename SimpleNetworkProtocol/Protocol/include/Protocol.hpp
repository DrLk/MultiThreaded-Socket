#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <utility>

#include "ByteStream.hpp"
#include "FileTree.hpp"
#include "FreeRecvPacketsJob.hpp"
#include "IConnection.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "MainJob.hpp"
#include "MainReadJob.hpp"
#include "MessageReader.hpp"
#include "MessageType.hpp"
#include "MessageWriter.hpp"
#include "ReadNetworkJob.hpp"
#include "Stream.hpp"
#include "TaskScheduler.hpp"
#include "WriteNetworkJob.hpp"

namespace FastTransport::TaskQueue {

class SendNetworkJob : public WriteNetworkJob {
    using Message = Protocol::IPacket::List;

public:
    static std::unique_ptr<SendNetworkJob> Create(Message&& message)
    {
        return std::make_unique<SendNetworkJob>(std::move(message));
    }

    SendNetworkJob(Message&& message)
        : _message(std::move(message))
    {
    }

    void ExecuteWriteNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) override
    {
        connection.Close();
        connection.Send(std::move(_message));
    }

private:
    Message _message;
};

class MergeOut : public MainJob {
public:
    static auto Create(FastTransport::FileSystem::FileTree& fileTree)
    {
        return std::make_unique<MergeOut>(fileTree);
    }

    MergeOut(FastTransport::FileSystem::FileTree& fileTree)
        : _fileTree(fileTree)
    {
    }

    Message ExecuteMain(std::stop_token stop, ITaskScheduler& scheduler, Message&& message) override
    {
        Protocol::MessageWriter writer(std::move(message));
        writer << MessageType::ResponseTree;
        FileSystem::OutputByteStream<Protocol::MessageWriter> output(writer);
        _fileTree.get().Serialize(output);

        scheduler.Schedule(SendNetworkJob::Create(writer.GetWritedPackets()));
        return message;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
};

class MergeIn : public MainReadJob {
    using MessageReader = Protocol::MessageReader;

public:
    static std::unique_ptr<MergeIn> Create(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader)
    {
        return std::make_unique<MergeIn>(fileTree, std::move(reader));
    }

    MergeIn(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader)
        : _fileTree(fileTree)
        , _reader(std::move(reader))
    {
    }

    void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) override
    {
        Protocol::MessageReader reader(std::move(_reader));
        FileSystem::InputByteStream<Protocol::MessageReader> input(reader);

        _fileTree.get().Deserialize(input);

        scheduler.Schedule(FreeRecvPacketsJob::Create(_reader.GetPackets()));
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    MessageReader _reader;
};

class MessageTypeReadJob : public ReadNetworkJob {
public:
    static std::unique_ptr<MessageTypeReadJob> Create(FastTransport::FileSystem::FileTree& fileTree, Message&& messages)
    {
        return std::make_unique<MessageTypeReadJob>(fileTree, std::move(messages));
    }

    MessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages)
        : _fileTree(fileTree)
        , _messages(std::move(messages))
    {
    }

    void ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection)
    {
        MessageType type;

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

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    Message _messages;
};
} // namespace FastTransport::TaskQueue
