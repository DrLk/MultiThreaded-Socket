#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include "ByteStream.hpp"
#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "NetworkJob.hpp"
#include "Stream.hpp"
#include "TaskScheduler.hpp"

namespace Jobs {

template <FastTransport::FileSystem::InputStream InputStream, FastTransport::FileSystem::OutputStream OutputStream>
class Dispatcher final {
    void AddJob(std::unique_ptr<TaskQueue::Job>&& job)
    {
        _taskscheduler.Schedule(TaskQueue::TaskType::Main, std::move(job));
    }

private:
    TaskQueue::TaskScheduler _taskscheduler;
    FastTransport::FileSystem::FileTree _fileTree;
    FastTransport::FileSystem::InputByteStream<InputStream> _input;
    FastTransport::FileSystem::OutputByteStream<OutputStream> _output;
};

enum class MessageType {
    None = 0,
    RequestTree = 1,
    ResponseTree = 2,
    RequestFile = 3,
    ResponseFile = 4,
    RequestFileBytes = 5,
    ResponseFileBytes = 6,
    RequestCloseFile = 7,
    ResponseCloseFile = 8,
};

template <class Message>
class WriteNetowrkJob : public TaskQueue::NetworkJob {
public:
    static std::unique_ptr<WriteNetowrkJob<Message>> Create(Message&& message)
    {
        return std::make_unique<WriteNetowrkJob<Message>>(std::forward<Message>(message));
    }

    WriteNetowrkJob(Message&& message)
        : _message(std::move(message))
    {
    }

    TaskQueue::TaskType Execute(TaskQueue::ITaskScheduler& scheduler, TaskQueue::Stream& output) override
    {
        //output << _message;
        return TaskQueue::TaskType::Main;
    }

    void ExecuteNetwork(TaskQueue::ITaskScheduler& scheduler, TaskQueue::Stream& stream) override
    {
        //stream << _message;
    }

private:
    Message _message;
};

template <FastTransport::FileSystem::OutputStream OutputStream, class Message>
class MergeOut : public TaskQueue::Job {
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

    void Accept(TaskQueue::ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskQueue::TaskType Execute(TaskQueue::ITaskScheduler& scheduler, TaskQueue::Stream& output) override
    {
        _output.get() << MessageType::ResponseTree;
        _fileTree.get().Serialize(_output.get());
        scheduler.Schedule(WriteNetowrkJob<Message>::Create(std::move(_message)));
        return TaskQueue::TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream<OutputStream>> _output;
    Message _message;
};

template <FastTransport::FileSystem::InputStream InputStream>
class MergeIn : public TaskQueue::Job {
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

    void Accept(TaskQueue::ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskQueue::TaskType Execute(TaskQueue::ITaskScheduler& scheduler, TaskQueue::Stream& input) override
    {
        _fileTree.get().Deserialize(_input.get());
        return TaskQueue::TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<InputStream>> _input;
};

template <FastTransport::FileSystem::InputStream InputStream, FastTransport::FileSystem::OutputStream OutputStream>
class MessageTypeReadJob : public TaskQueue::Job {
public:
    MessageTypeReadJob(FastTransport::FileSystem::InputByteStream<InputStream>& input, FastTransport::FileSystem::OutputByteStream<OutputStream>& output, FastTransport::FileSystem::FileTree& fileTree)
        : _input(input)
        , _output(output)
        , _fileTree(fileTree)
    {
    }

    void Accept(TaskQueue::ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override
    {
    }

    TaskQueue::TaskType Execute(TaskQueue::ITaskScheduler& scheduler, TaskQueue::Stream& output) override
    {
        MessageType type;
        _input.get() >> type;
        switch (type) {
        case MessageType::RequestTree:
            // scheduler.Schedule(TaskQueue::TaskType::Main, MergeOut<OutputStream>::Create(_fileTree, _output));
            break;
        case MessageType::ResponseTree:
            scheduler.Schedule(TaskQueue::TaskType::Main, MergeIn<OutputStream>::Create(_fileTree, _input));
            break;
        case MessageType::RequestFile:
            break;
        case MessageType::ResponseFile:
            break;
        default:
            throw std::runtime_error("MessageTypeReadJob: unknown message type");
        }
        return TaskQueue::TaskType::Main;
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<InputStream>> _input;
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream<OutputStream>> _output;
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
};
}
