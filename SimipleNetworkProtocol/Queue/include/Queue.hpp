#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <condition_variable>
#include "LockedList.hpp"

namespace TaskQueue {

class TaskQueue {
public:
    std::future<void> Async(std::stop_token stop, std::packaged_task<void()>&&);

private:
    static constexpr int MaxTaskQueueSize = 100;
    FastTransport::Containers::LockedList<std::packaged_task<void()>> _taskQueue;

    static void ProcessQueue(TaskQueue& taskQueue);
};

int64_t add(int64_t lhs, int64_t rhs);
int64_t substract(int64_t lhs, int64_t rhs);
int64_t multiply(int64_t lhs, int64_t rhs);
int64_t divide(int64_t lhs, int64_t rhs);
int64_t square(int64_t value);
} // namespace calc