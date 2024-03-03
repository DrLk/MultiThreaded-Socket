#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <stop_token>

#include "LockedList.hpp"
#include "MultiList.hpp"

namespace TaskQueue {

class ITaskQueue {
    virtual std::future<void> Async(std::move_only_function<void()>&& function) = 0;
public:
    ITaskQueue() = default;
    ITaskQueue(const ITaskQueue&) = delete;
    ITaskQueue(ITaskQueue&&) = delete;
    ITaskQueue& operator=(const ITaskQueue&) = delete;
    ITaskQueue& operator=(ITaskQueue&&) = delete;
    virtual ~ITaskQueue() = default;
};

class TaskQueue : public ITaskQueue {
public:
    std::future<void> Async(std::move_only_function<void()>&& function) override;

private:
    using LockedList = FastTransport::Containers::LockedList<std::packaged_task<void()>>;
    using List = FastTransport::Containers::MultiList<std::packaged_task<void()>>;
    static constexpr int MaxTaskQueueSize = 100;
    LockedList _taskQueue;

    static void ProcessQueue(std::stop_token stop, TaskQueue& taskQueue);
};

int64_t add(int64_t lhs, int64_t rhs);
int64_t substract(int64_t lhs, int64_t rhs);
int64_t multiply(int64_t lhs, int64_t rhs);
int64_t divide(int64_t lhs, int64_t rhs);
int64_t square(int64_t value);
} // namespace TaskQueue
