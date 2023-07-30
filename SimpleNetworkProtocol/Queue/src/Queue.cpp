#include "Queue.hpp"

namespace TaskQueue {

std::future<void> TaskQueue::Async(std::stop_token stop, std::packaged_task<void()>&& task)
{
    std::future<void> future = task.get_future();
    _taskQueue.LockedPushBack(std::move(task));

    return future;
}
void TaskQueue::ProcessQueue(TaskQueue& taskQueue)
{
}

int64_t add(int64_t lhs, int64_t rhs)
{
    return lhs + rhs;
}

int64_t substract(int64_t lhs, int64_t rhs)
{
    return lhs - rhs;
}

int64_t multiply(int64_t lhs, int64_t rhs)
{
    return lhs * rhs;
}

int64_t divide(int64_t lhs, int64_t rhs)
{
    return lhs / rhs;
}

int64_t square(int64_t value)
{
    return value * value;
}
} // namespace calc