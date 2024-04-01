#include "OpenFileJob.hpp"

#include "File.hpp"

namespace FastTransport::TaskQueue {
OpenFileJob::OpenFileJob(File& file)
    : _file(file)
{
}

void OpenFileJob::ExecuteDisk(ITaskScheduler& scheduler, float disk)
{

    _file.Open();
}

} // namespace FastTransport::TaskQueue
