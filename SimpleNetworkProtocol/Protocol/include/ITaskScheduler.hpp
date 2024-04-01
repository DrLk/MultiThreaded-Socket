#pragma once

#include <memory>
#include <stop_token>

namespace FastTransport::TaskQueue {

class Job;
class WriteNetworkJob;
class ReadNetworkJob;
class DiskJob;
class MainJob;
class MainReadJob;
class FuseNetworkJob;

class ITaskScheduler {
public:
    ITaskScheduler() = default;
    ITaskScheduler(const ITaskScheduler&) = delete;
    ITaskScheduler(ITaskScheduler&&) = delete;
    ITaskScheduler& operator=(const ITaskScheduler&) = delete;
    ITaskScheduler& operator=(ITaskScheduler&&) = delete;
    virtual ~ITaskScheduler() = default;

    virtual void Schedule(std::unique_ptr<Job>&& job) = 0;
    virtual void Wait(std::stop_token stop) = 0;

    virtual void ScheduleMainJob(std::unique_ptr<MainJob>&& job) = 0;
    virtual void ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job) = 0;
    virtual void ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job) = 0;
    virtual void ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job) = 0;
    virtual void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) = 0;
    virtual void ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
