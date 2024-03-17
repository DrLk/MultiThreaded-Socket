#pragma once

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
public:
    static std::unique_ptr<MergeIn> Create(FastTransport::FileSystem::FileTree& fileTree, Message&& message)
    {
        return std::make_unique<MergeIn>(fileTree, std::move(message));
    }

    MergeIn(FastTransport::FileSystem::FileTree& fileTree, Message&& message)
        : _fileTree(fileTree)
        , _message(std::move(message))
    {
    }

    void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) override
    {
        Protocol::MessageReader reader(std::move(_message));
        FileSystem::InputByteStream<Protocol::MessageReader> input(reader);

        _fileTree.get().Deserialize(input);

        scheduler.Schedule(FreeRecvPacketsJob::Create(std::move(_message)));
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    Message _message;
};

template <FastTransport::FileSystem::InputStream InputStream, FastTransport::FileSystem::OutputStream OutputStream>
class MessageTypeReadJob : public ReadNetworkJob {
public:
    MessageTypeReadJob(FastTransport::FileSystem::InputByteStream<InputStream>& input, FastTransport::FileSystem::OutputByteStream<OutputStream>& output, FastTransport::FileSystem::FileTree& fileTree)
        : _input(input)
        , _output(output)
        , _fileTree(fileTree)
    {
    }

    Message ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection)
    {
        MessageType type;
        _input.get() >> type;
        Message message;
        switch (type) {
        case MessageType::RequestTree:
            scheduler.Schedule(MergeOut::Create(_fileTree));
            break;
        case MessageType::ResponseTree:
            scheduler.Schedule(MergeIn::Create(_fileTree, std::move(message)));
            break;
        case MessageType::RequestFile:
            break;
        case MessageType::ResponseFile:
            break;
        default:
            throw std::runtime_error("MessageTypeReadJob: unknown message type");
        }
        return message;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<InputStream>> _input;
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream<OutputStream>> _output;
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
};
} // namespace FastTransport::TaskQueue
