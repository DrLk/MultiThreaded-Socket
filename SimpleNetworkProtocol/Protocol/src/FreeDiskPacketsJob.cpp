#include "FreeDiskPacketsJob.hpp"

namespace FastTransport::TaskQueue {
    FreeDiskPacketsJob::FreeDiskPacketsJob(Data&& data)
        : _data(std::move(data))
    {
    }

    FreeDiskPacketsJob::Data FreeDiskPacketsJob::ExecuteDisk(ITaskScheduler& /*scheduler*/, Data&& free)
    {
        free.splice(std::move(_data));
        return std::move(free);
    }

} // namespace FastTransport::TaskQueue
