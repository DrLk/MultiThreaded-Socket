#pragma once

#include "IServerTaskScheduler.hpp"
#include "TaskScheduler.hpp"

namespace FastTransport::TaskQueue {

class ServerTaskScheduler final : public TaskSchedulerBase, public IServerTaskScheduler {
public:
    ServerTaskScheduler(IConnection& connection, FileTree& fileTree);
    ServerTaskScheduler(const ServerTaskScheduler&) = delete;
    ServerTaskScheduler(ServerTaskScheduler&&) = delete;
    ServerTaskScheduler& operator=(const ServerTaskScheduler&) = delete;
    ServerTaskScheduler& operator=(ServerTaskScheduler&&) = delete;
    ~ServerTaskScheduler() override;

    void ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job) override;
    void ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job) override;
    void ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job) override;

private:
    // Dedicated queue for InotifyWatcherJob so it doesn't block the mainQueue.
    TaskQueue _inotifyQueue;
};

} // namespace FastTransport::TaskQueue
