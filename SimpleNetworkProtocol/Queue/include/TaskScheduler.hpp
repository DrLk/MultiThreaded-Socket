#pragma once

#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Queue.hpp"
#include "TaskType.hpp"

namespace TaskQueue {

class TaskScheduler final : public ITaskScheduler {
public:
    TaskScheduler();
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    ~TaskScheduler() override;

    void Schedule(TaskType type, std::unique_ptr<Job>&& job) override;

private:
    TaskQueue _diskQueue;
    TaskQueue _mainQueue;
};

} // namespace TaskQueue
