#include "FuseNetworkJob.hpp"

#include <memory>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void FuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<FuseNetworkJob*>(job.release());
    std::unique_ptr<FuseNetworkJob> fuseNetworkJob(pointer);
    scheduler.ScheduleFuseNetworkJob(std::move(fuseNetworkJob));
}

void FuseNetworkJob::InitReader(Reader&& reader)
{
    _reader = std::move(reader);
}

FuseNetworkJob::Reader& FuseNetworkJob::GetReader()
{
    return _reader;
}

FuseNetworkJob::Message FuseNetworkJob::GetFreeReadPackets()
{
    return _reader.GetPackets();
}

FuseNetworkJob::FileHandle& FuseNetworkJob::GetFileHandle(fuse_file_info* fileInfo)
{
    return *(reinterpret_cast<FileHandle*>(fileInfo->fh)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

} // namespace FastTransport::TaskQueue
