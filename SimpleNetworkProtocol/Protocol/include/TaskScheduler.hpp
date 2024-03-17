#pragma once

#include "IConnection.hpp"
#include "IPacket.hpp"
#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Queue.hpp"

namespace FastTransport::TaskQueue {

class Stream;

class TaskScheduler final : public ITaskScheduler {
    using IConnection = FastTransport::Protocol::IConnection;
    using Message = FastTransport::Protocol::IPacket::List;

public:
    TaskScheduler(Stream& diskStream, IConnection& connection);
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    ~TaskScheduler() override;

    void Schedule(std::unique_ptr<Job>&& job) override;
    void ScheduleMainJob(std::unique_ptr<MainJob>&& job) override;
    void ScheduleMainReadJob(std::unique_ptr<MainReadJob>&& job) override;
    void ScheduleWriteNetworkJob(std::unique_ptr<WriteNetworkJob>&& job) override;
    void ScheduleReadNetworkJob(std::unique_ptr<ReadNetworkJob>&& job) override;
    void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) override;

private:
    TaskQueue _diskQueue;
    std::reference_wrapper<Stream> _diskStream;
    TaskQueue _mainQueue;
    std::reference_wrapper<IConnection> _connection;
    Message _freeSendPackets;
    Message _freeRecvPackets;
};

} // namespace FastTransport::TaskQueue
