#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <stop_token>
#include <utility>

#include "ByteStream.hpp"
#include "FileTree.hpp"
#include "IConnection.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "MessageType.hpp"
#include "NetworkJob.hpp"
#include "Stream.hpp"
#include "TaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class WriteNetowrkJob : public NetworkJob {
    using Message = Protocol::IPacket::List;

public:
    static std::unique_ptr<WriteNetowrkJob> Create(Message&& message)
    {
        return std::make_unique<WriteNetowrkJob>(std::move(message));
    }

    WriteNetowrkJob(Message&& message)
        : _message(std::move(message))
    {
    }

    TaskType Execute(ITaskScheduler& scheduler, Stream& output) override
    {
        //output << _message;
        return TaskType::Main;
    }

    void ExecuteNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) override
    {
        connection.Close();
        auto freePackets = connection.Send(stop, std::move(_message));
    }

private:
    Message _message;
};

template <FastTransport::FileSystem::OutputStream OutputStream, class Message>
class MergeOut : public Job {
public:
    static auto Create(FastTransport::FileSystem::FileTree& fileTree, FastTransport::FileSystem::OutputByteStream<OutputStream>& output)
    {
        return std::make_unique<MergeOut<OutputStream, Message>>(fileTree, output);
    }

    MergeOut(FastTransport::FileSystem::FileTree& fileTree, FastTransport::FileSystem::OutputByteStream<OutputStream>& output, Message&& messagae)
        : _fileTree(fileTree)
        , _output(output)
        , _message(std::move(messagae))
    {
    }

    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskType Execute(ITaskScheduler& scheduler, Stream& output) override
    {
        _output.get() << MessageType::ResponseTree;
        _fileTree.get().Serialize(_output.get());
        scheduler.Schedule(WriteNetowrkJob::Create(std::move(_message)));
        return TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream<OutputStream>> _output;
    Message _message;
};

template <FastTransport::FileSystem::InputStream InputStream>
class MergeIn : public Job {
public:
    static std::unique_ptr<MergeIn<InputStream>> Create(FastTransport::FileSystem::FileTree& fileTree, FastTransport::FileSystem::InputByteStream<InputStream>& input)
    {
        return std::make_unique<MergeIn<InputStream>>(fileTree, input);
    }

    MergeIn(FastTransport::FileSystem::FileTree& fileTree, FastTransport::FileSystem::InputByteStream<InputStream>& input)
        : _fileTree(fileTree)
        , _input(input)
    {
    }

    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskType Execute(ITaskScheduler& scheduler, Stream& input) override
    {
        _fileTree.get().Deserialize(_input.get());
        return TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<InputStream>> _input;
};

template <FastTransport::FileSystem::InputStream InputStream, FastTransport::FileSystem::OutputStream OutputStream>
class MessageTypeReadJob : public Job {
public:
    MessageTypeReadJob(FastTransport::FileSystem::InputByteStream<InputStream>& input, FastTransport::FileSystem::OutputByteStream<OutputStream>& output, FastTransport::FileSystem::FileTree& fileTree)
        : _input(input)
        , _output(output)
        , _fileTree(fileTree)
    {
    }

    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskType Execute(ITaskScheduler& scheduler, Stream& output) override
    {
        MessageType type;
        _input.get() >> type;
        switch (type) {
        case MessageType::RequestTree:
            // scheduler.Schedule(TaskType::Main, MergeOut<OutputStream>::Create(_fileTree, _output));
            break;
        case MessageType::ResponseTree:
            scheduler.Schedule(TaskType::Main, MergeIn<OutputStream>::Create(_fileTree, _input));
            break;
        case MessageType::RequestFile:
            break;
        case MessageType::ResponseFile:
            break;
        default:
            throw std::runtime_error("MessageTypeReadJob: unknown message type");
        }
        return TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<InputStream>> _input;
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream<OutputStream>> _output;
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
};
} // namespace FastTransport::TaskQueue
