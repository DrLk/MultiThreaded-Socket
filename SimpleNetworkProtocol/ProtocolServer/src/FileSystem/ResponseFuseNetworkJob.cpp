#include "ResponseFuseNetworkJob.hpp"

#include <cstdint>
#include <memory>

#include "IServerTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "Inode.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void ResponseFuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    // See FuseNetworkJob::Accept for rationale on static_cast over dynamic_cast.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    std::unique_ptr<ResponseFuseNetworkJob> fuseNetworkJob(static_cast<ResponseFuseNetworkJob*>(job.release()));
    dynamic_cast<IServerTaskScheduler&>(scheduler).ScheduleResponseFuseNetworkJob(std::move(fuseNetworkJob));
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

ResponseFuseNetworkJob::Leaf& ResponseFuseNetworkJob::GetLeaf(std::uint64_t inode, FileTree& fileTree)
{
    return inode == FastTransport::FileSystem::RootInode ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

std::uint64_t ResponseFuseNetworkJob::GetINode(const Leaf& leaf)
{
    return reinterpret_cast<std::uint64_t>(&leaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

} // namespace FastTransport::TaskQueue
