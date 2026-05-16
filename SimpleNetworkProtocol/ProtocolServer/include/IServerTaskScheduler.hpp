#pragma once

#include <memory>

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class ResponseFuseNetworkJob;
class ResponseReadFileJob;
class InotifyWatcherJob;

class IServerTaskScheduler : public virtual ITaskScheduler {
public:
    IServerTaskScheduler() = default;
    IServerTaskScheduler(const IServerTaskScheduler&) = delete;
    IServerTaskScheduler(IServerTaskScheduler&&) = delete;
    IServerTaskScheduler& operator=(const IServerTaskScheduler&) = delete;
    IServerTaskScheduler& operator=(IServerTaskScheduler&&) = delete;
    ~IServerTaskScheduler() override = default;

    virtual void ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job) = 0;
    virtual void ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job) = 0;
    virtual void ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
