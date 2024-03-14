#include "Queue.hpp"

#include <stop_token>
#include <utility>

namespace FastTransport::TaskQueue {

std::future<void> TaskQueue::Async(std::move_only_function<void(std::stop_token)>&& function)
{
    auto task = std::packaged_task<void(std::stop_token)>(std::move(function));
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
        task(stop);
    }
}
} // namespace FastTransport::TaskQueue
