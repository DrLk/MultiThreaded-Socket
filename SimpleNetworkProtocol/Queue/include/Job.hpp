#pragma once

#include "TaskType.hpp"

namespace TaskQueue {
class ITaskScheduler;

class Job {
public:
    Job() = default;
    Job(const Job&) = delete;
    Job(Job&&) = default;
    Job& operator=(const Job&) = delete;
    Job& operator=(Job&&) = default;
    virtual ~Job() = default;
    virtual TaskType Execute(ITaskScheduler& scheduler) = 0;
};
} // namespace TaskQueue
