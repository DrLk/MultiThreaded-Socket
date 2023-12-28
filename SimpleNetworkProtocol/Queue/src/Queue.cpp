#include "Queue.hpp"

#include <utility>


namespace TaskQueue {

std::future<void> TaskQueue::Async(std::function<void()>&& function)
{
    auto task = std::packaged_task<void()>(std::move(function));
    std::future<void> future = task.get_future();
    _taskQueue.LockedPushBack(std::move(task));
    _taskQueue.NotifyAll();

    return future;
}
void TaskQueue::ProcessQueue(std::stop_token stop, TaskQueue& queue)
{
    List taskQueue;

    queue._taskQueue.Wait(stop);
    if (stop.stop_requested()) {
        return;
    }

    queue._taskQueue.LockedSwap(taskQueue);

    for (auto& task : taskQueue) {
        task();
    }
}

int64_t add(int64_t lhs, int64_t rhs)
{
    TaskQueue queue;
    const std::future wait = queue.Async([]() { return; });
    wait.wait();
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
}  // namespace TaskQueue
