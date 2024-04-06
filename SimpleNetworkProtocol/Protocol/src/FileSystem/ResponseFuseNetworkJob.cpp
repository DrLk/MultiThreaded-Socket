#include "ResponseFuseNetworkJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <memory>

#include "ITaskScheduler.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void ResponseFuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<ResponseFuseNetworkJob*>(job.release());
    std::unique_ptr<ResponseFuseNetworkJob> fuseNetworkJob(pointer);
    scheduler.ScheduleResponseFuseNetworkJob(std::move(fuseNetworkJob));
}

void ResponseFuseNetworkJob::InitReader(Reader&& reader)
{
    _reader = std::move(reader);
}

ResponseFuseNetworkJob::Reader& ResponseFuseNetworkJob::GetReader()
{
    return _reader;
}

ResponseFuseNetworkJob::Message ResponseFuseNetworkJob::GetFreeReadPackets()
{
    return _reader.GetPackets();
}

ResponseFuseNetworkJob::Leaf& ResponseFuseNetworkJob::GetLeaf(fuse_ino_t inode, FileTree& fileTree)
{

    return inode == FUSE_ROOT_ID ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

fuse_ino_t ResponseFuseNetworkJob::GetINode(const Leaf& leaf)
{
    return reinterpret_cast<fuse_ino_t>(&leaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

} // namespace FastTransport::TaskQueue
