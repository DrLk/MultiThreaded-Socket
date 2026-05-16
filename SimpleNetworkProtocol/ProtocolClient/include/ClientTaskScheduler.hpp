#pragma once

#include "IClientTaskScheduler.hpp"
#include "TaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class ClientTaskScheduler final : public TaskSchedulerBase, public IClientTaskScheduler {
public:
    ClientTaskScheduler(IConnection& connection, FileTree& fileTree);
    ClientTaskScheduler(const ClientTaskScheduler&) = delete;
    ClientTaskScheduler(ClientTaskScheduler&&) = delete;
    ClientTaskScheduler& operator=(const ClientTaskScheduler&) = delete;
    ClientTaskScheduler& operator=(ClientTaskScheduler&&) = delete;
    ~ClientTaskScheduler() override;

    void ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job) override;
    void ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job) override;
};

} // namespace FastTransport::TaskQueue
