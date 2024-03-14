#pragma once

#include <memory>

#include "Stream.hpp"
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
    virtual void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) = 0;
    virtual TaskType Execute(ITaskScheduler& scheduler, Stream& stream) = 0;
};

} // namespace TaskQueue
