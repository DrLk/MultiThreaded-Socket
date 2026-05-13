#include "FreeDiskPacketsJob.hpp"
#include "IPacket.hpp"
#include <Tracy.hpp>

namespace FastTransport::TaskQueue {
FreeDiskPacketsJob::FreeDiskPacketsJob(Data&& data)
    : _data(std::move(data))
{
}

FreeDiskPacketsJob::Data FreeDiskPacketsJob::ExecuteDisk(ITaskScheduler& /*scheduler*/, Data&& free)
{
    ZoneScopedN("FreeDiskPacketsJob::ExecuteDisk");
    free.splice(std::move(_data));
    return std::move(free);
}

} // namespace FastTransport::TaskQueue
