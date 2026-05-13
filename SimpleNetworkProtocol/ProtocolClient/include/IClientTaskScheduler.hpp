#pragma once

#include <memory>

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class FuseNetworkJob;
class ResponseInFuseNetworkJob;

class IClientTaskScheduler : public virtual ITaskScheduler {
public:
    IClientTaskScheduler() = default;
    IClientTaskScheduler(const IClientTaskScheduler&) = delete;
    IClientTaskScheduler(IClientTaskScheduler&&) = delete;
    IClientTaskScheduler& operator=(const IClientTaskScheduler&) = delete;
    IClientTaskScheduler& operator=(IClientTaskScheduler&&) = delete;
    ~IClientTaskScheduler() override = default;

    virtual void ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job) = 0;
    virtual void ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job) = 0;
};

} // namespace FastTransport::TaskQueue
