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

// Common scheduler base. Holds the queues / state that are shared between the
// server and client schedulers. Concrete derived classes (ServerTaskScheduler,
// ClientTaskScheduler) are defined in ProtocolServer/ProtocolClient and add
// the side-specific queues + Schedule methods.
class TaskSchedulerBase : public virtual ITaskScheduler {
protected:
    using IConnection = FastTransport::Protocol::IConnection;
    using Message = FastTransport::Protocol::IPacket::List;
    using FileTree = FileSystem::FileTree;

public:
    TaskSchedulerBase(IConnection& connection, FileTree& fileTree);
    TaskSchedulerBase(const TaskSchedulerBase&) = delete;
    TaskSchedulerBase(TaskSchedulerBase&&) = delete;
    TaskSchedulerBase& operator=(const TaskSchedulerBase&) = delete;
    TaskSchedulerBase& operator=(TaskSchedulerBase&&) = delete;
    ~TaskSchedulerBase() override;

    void Schedule(std::unique_ptr<Job>&& job) override;
    void Wait(std::stop_token stop) override;

    void ScheduleMainJob(std::unique_ptr<MainJob>&& job) override;
    void ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job) override;
    void ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job) override;
    void ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job) override;
    void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) override;
    void ReturnFreeDiskPackets(FastTransport::Protocol::IPacket::List&& packets) override;

protected:
    // Stop and join the common queues. Derived destructors must call this BEFORE
    // they return, so worker threads cannot make virtual calls on `*this` while
    // the vptr transitions from the derived to the base class. Idempotent.
    void ShutdownCommonQueues() noexcept;

    static constexpr size_t DiskThreadCount = 4;

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
};

} // namespace FastTransport::TaskQueue
