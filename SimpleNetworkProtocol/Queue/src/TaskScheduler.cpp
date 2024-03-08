#include "TaskScheduler.hpp"

namespace TaskQueue {

TaskScheduler::TaskScheduler() = default;

TaskScheduler::~TaskScheduler() = default;

void TaskScheduler::Schedule(TaskType type, std::unique_ptr<Job>&& job)
{
    switch (type) {
    case TaskType::Main: {
        _mainQueue.Async([job = std::move(job), this]() mutable {
            const TaskType type = job->Execute(*this);
            Schedule(type, std::move(job));
        });
        break;
    }
    case TaskType::Disk: {
        _diskQueue.Async([job = std::move(job), this]() mutable {
            const TaskType type = job->Execute(*this);
            Schedule(type, std::move(job));
        });
        break;
    }
    case TaskType::None: {
        break;
    }
    }
}

} // namespace TaskQueue
