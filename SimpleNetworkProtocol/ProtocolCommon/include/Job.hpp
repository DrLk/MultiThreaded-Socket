#pragma once

#include <memory>

namespace FastTransport::TaskQueue {
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
};

} // namespace FastTransport::TaskQueue
