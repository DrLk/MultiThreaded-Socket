#pragma once

#include <array>

#include "FileTree.hpp"
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
    void ScheduleCacheTreeJob(std::unique_ptr<CacheTreeJob>&& job) override;

private:
    static constexpr size_t CacheTreeThreadCount = FileSystem::FileTree::ShardCount;

    // Dedicated queue for CacheTreeJob (FuseReadFileJob, ApplyBlockCacheJob).
    // Keeps leaf/tree operations off the mainQueue so network responses are
    // processed without stalling behind fuse_reply_data calls.
    std::array<TaskQueue, CacheTreeThreadCount> _cacheTreeQueues;
};

} // namespace FastTransport::TaskQueue
