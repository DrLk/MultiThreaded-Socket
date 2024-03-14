#pragma once

#include <memory>

namespace FastTransport::TaskQueue {

class Job;
class NetworkJob;
class DiskJob;

class ITaskScheduler {
public:
    ITaskScheduler() = default;
    ITaskScheduler(const ITaskScheduler&) = delete;
    ITaskScheduler(ITaskScheduler&&) = delete;
    ITaskScheduler& operator=(const ITaskScheduler&) = delete;
    ITaskScheduler& operator=(ITaskScheduler&&) = delete;
    virtual ~ITaskScheduler() = default;

    virtual void Schedule(std::unique_ptr<Job>&& job) = 0;

    virtual void ScheduleNetworkJob(std::unique_ptr<NetworkJob>&& job) = 0;
    virtual void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
