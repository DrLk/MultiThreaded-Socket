#include "Queue.hpp"

#include <memory>
#include <utility>

#include "httplib.hpp"

namespace TaskQueue {

std::future<void> TaskQueue::Async(std::stop_token stop, std::function<void()>&& function)
{
    auto task = std::packaged_task<void()>(function);
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
    std::future wait = queue.Async(std::stop_token(), []() { return; });
    wait.wait();
    return lhs + rhs;
}

int64_t substract(int64_t lhs, int64_t rhs)
{
    // HTTP
    //httplib::Client cli("http://www.columbia.edu");

    // HTTPS
    httplib::Client cli("https://habr.com");

    cli.Get("/ru/all/");

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
