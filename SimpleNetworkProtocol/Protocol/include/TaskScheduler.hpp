#pragma once

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
    void ScheduleResponseInFuseNetworkJob(std::unique_ptr<ResponseInFuseNetworkJob>&& job) override;
    void ScheduleCacheTreeJob(std::unique_ptr<CacheTreeJob>&& job) override;
    void ReturnFreeDiskPackets(FastTransport::Protocol::IPacket::List&& packets) override;

private:
    // Data members must be declared before queues so that queues (and their
    // worker threads) are destroyed first, before the data they access.
    std::reference_wrapper<IConnection> _connection;
    std::reference_wrapper<FileTree> _fileTree;
    Message _freeDiskPackets;
    Message _freeSendPackets;

    // Queues are destroyed in reverse order: _diskQueue first, then _mainQueue,
    // then _readNetworkQueue/_writeNetworkQueue. Each join() ensures no more
    // access to the data members above.
    TaskQueue _writeNetworkQueue;
    TaskQueue _readNetworkQueue;
    TaskQueue _mainQueue;
    TaskQueue _diskQueue;
};

} // namespace FastTransport::TaskQueue
