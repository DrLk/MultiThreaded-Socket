#pragma once

#include <memory>

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class FuseNetworkJob;
class ResponseInFuseNetworkJob;
class CacheTreeJob;

class IClientTaskScheduler : public virtual ITaskScheduler {
public:
    virtual void ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job) = 0;
    virtual void ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job) = 0;
    virtual void ScheduleCacheTreeJob(std::unique_ptr<CacheTreeJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
