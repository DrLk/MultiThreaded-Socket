#pragma once

#include <array>
#include <atomic>

#include "FileTree.hpp"
#include "IConnection.hpp"
#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Queue.hpp"

namespace FastTransport::TaskQueue {

class TaskScheduler final : public ITaskScheduler {
    using IConnection = FastTransport::Protocol::IConnection;
    using Message = FastTransport::Protocol::IPacket::List;
    using FileTree = FileSystem::FileTree;

public:
    TaskScheduler(IConnection& connection, FileTree& fileTree);
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    ~TaskScheduler() override;

    void Schedule(std::unique_ptr<Job>&& job) override;
    void Wait(std::stop_token stop) override;

    void ScheduleMainJob(std::unique_ptr<MainJob>&& job) override;
    void ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job) override;
    void ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job) override;
    void ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job) override;
    void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) override;
    void ScheduleFuseNetworkJob(std::unique_ptr<FuseNetworkJob>&& job) override;
    void ScheduleResponseFuseNetworkJob(std::unique_ptr<ResponseFuseNetworkJob>&& job) override;
    void ScheduleResponseReadDiskJob(std::unique_ptr<ResponseReadFileJob>&& job) override;
    void ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job) override;
    void ScheduleCacheTreeJob(std::unique_ptr<CacheTreeJob>&& job) override;
    void ScheduleInotifyWatcherJob(std::unique_ptr<InotifyWatcherJob>&& job) override;
    void ReturnFreeDiskPackets(FastTransport::Protocol::IPacket::List&& packets) override;

private:
    static constexpr size_t DiskThreadCount = 4;
    static constexpr size_t CacheTreeThreadCount = FileSystem::FileTree::ShardCount;

    // Data members must be declared before queues so that queues (and their
    // worker threads) are destroyed first, before the data they access.
    std::reference_wrapper<IConnection> _connection;
    std::reference_wrapper<FileTree> _fileTree;
    std::array<Message, DiskThreadCount> _freeDiskPackets;
    Message _freeSendPackets;
    std::atomic<size_t> _nextDiskQueue { 0 };

    // The destructor requests stop on all queues and joins all workers before
    // any of these members are destroyed, so no worker can race with the
    // LockedList destructors inside each TaskQueue.
    TaskQueue _writeNetworkQueue;
    TaskQueue _readNetworkQueue;
    TaskQueue _mainQueue;
    std::array<TaskQueue, DiskThreadCount> _diskQueues;
    // Dedicated queue for CacheTreeJob (FuseReadFileJob, ApplyBlockCacheJob).
    // Keeps leaf/tree operations off the mainQueue so network responses are
    // processed without stalling behind fuse_reply_data calls.
    std::array<TaskQueue, CacheTreeThreadCount> _cacheTreeQueues;
    // Dedicated queue for InotifyWatcherJob so it doesn't block the mainQueue.
    TaskQueue _inotifyQueue;
};

} // namespace FastTransport::TaskQueue
