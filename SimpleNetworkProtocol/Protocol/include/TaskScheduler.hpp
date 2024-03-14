#pragma once

#include "ITaskScheduler.hpp"
#include "Job.hpp"
#include "Queue.hpp"
#include "TaskType.hpp"

namespace TaskQueue {

class Stream;

class TaskScheduler final : public ITaskScheduler {
public:
    TaskScheduler(Stream& diskStream, Stream& networkStream);
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    ~TaskScheduler() override;

    void Schedule(std::unique_ptr<Job>&& job) override;
    void Schedule(TaskType type, std::unique_ptr<Job>&& job) override;
    void ScheduleNetworkJob(std::unique_ptr<NetworkJob>&& job) override;
    void ScheduleDiskJob(std::unique_ptr<DiskJob>&& job) override;

        private : TaskQueue _diskQueue;
    std::reference_wrapper<Stream> _diskStream;
    TaskQueue _mainQueue;
    std::reference_wrapper<Stream> _networkStream;
};

} // namespace TaskQueue