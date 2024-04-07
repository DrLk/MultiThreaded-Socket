#include "Queue.hpp"

#include <stop_token>
#include <utility>

#include "Logger.hpp"

namespace FastTransport::TaskQueue {

TaskQueue::TaskQueue()
    : _workerThread(ProcessQueue, std::ref(*this))
{
}

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
    while (queue._taskQueue.Wait(stop)) {
        List taskQueue;

        queue._taskQueue.LockedSwap(taskQueue);
        LOGGER() << "Task queue size: " << taskQueue.size();

        for (auto& task : taskQueue) {
            task(stop);
        }

        queue._taskQueue.NotifyAll();
    }
}
} // namespace FastTransport::TaskQueue
