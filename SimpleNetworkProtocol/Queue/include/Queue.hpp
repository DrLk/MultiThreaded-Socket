#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <stop_token>

#include "LockedList.hpp"
#include "MultiList.hpp"

namespace TaskQueue {

class ITaskQueue {
    virtual std::future<void> Async(std::stop_token stop, std::function<void()>&& task) = 0;
};

class TaskQueue : public ITaskQueue {
public:
    std::future<void> Async(std::stop_token stop, std::function<void()>&& task) override;

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