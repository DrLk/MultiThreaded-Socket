#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <stop_token>
#include <thread>

#include "LockedList.hpp"
#include "MultiList.hpp"

namespace FastTransport::TaskQueue {

class ITaskQueue {
    virtual std::future<void> Async(std::move_only_function<void(std::stop_token)>&& function) = 0;
public:
    ITaskQueue() = default;
    ITaskQueue(const ITaskQueue&) = delete;
    ITaskQueue(ITaskQueue&&) = delete;
    ITaskQueue& operator=(const ITaskQueue&) = delete;
    ITaskQueue& operator=(ITaskQueue&&) = delete;
    virtual ~ITaskQueue() = default;
};

class TaskQueue : public ITaskQueue {
private:
    using LockedList = FastTransport::Containers::LockedList<std::packaged_task<void(std::stop_token)>>;
    using List = FastTransport::Containers::MultiList<std::packaged_task<void(std::stop_token)>>;

    static constexpr int MaxTaskQueueSize = 100;

public:
    TaskQueue();
    std::future<void> Async(std::move_only_function<void(std::stop_token)>&& function) override;

private:
    LockedList _taskQueue;
    std::jthread _workerThread;

    static void ProcessQueue(std::stop_token stop, TaskQueue& taskQueue);
};

} // namespace FastTransport::TaskQueue
