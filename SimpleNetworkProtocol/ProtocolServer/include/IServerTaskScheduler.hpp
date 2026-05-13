#pragma once

#include <memory>

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class ResponseFuseNetworkJob;
class ResponseReadFileJob;
class InotifyWatcherJob;

class IServerTaskScheduler : public virtual ITaskScheduler {
public:
    virtual void ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job) = 0;
    virtual void ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job) = 0;
    virtual void ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
