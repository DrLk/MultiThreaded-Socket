#pragma once

#include "DiskJob.hpp"
#include "MultiList.hpp"

namespace FastTransport::TaskQueue {

class ITaskScheduler;

class FreeDiskPacketsJob : public DiskJob {
public:
    explicit FreeDiskPacketsJob(Data&& data);
    Data ExecuteDisk(ITaskScheduler& scheduler, Data&& free) override;

private:
    Data _data;
};

} // namespace FastTransport::TaskQueue
