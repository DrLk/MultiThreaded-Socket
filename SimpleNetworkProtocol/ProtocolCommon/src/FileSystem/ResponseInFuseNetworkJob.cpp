#include "ResponseInFuseNetworkJob.hpp"

#include <cstdint>
#include <fuse3/fuse_lowlevel.h>
#include <memory>

#include "FileTree.hpp"
#include "IClientTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "Inode.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

void ResponseInFuseNetworkJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    auto* pointer = dynamic_cast<ResponseInFuseNetworkJob*>(job.release());
    std::unique_ptr<ResponseInFuseNetworkJob> fuseNetworkJob(pointer);
    dynamic_cast<IClientTaskScheduler&>(scheduler).ScheduleResponseInFuseNetworkJob(std::move(fuseNetworkJob));
}

void ResponseInFuseNetworkJob::InitReader(Reader&& reader)
{
    _reader = std::move(reader);
}

ResponseInFuseNetworkJob::Reader& ResponseInFuseNetworkJob::GetReader()
{
    return _reader;
}

ResponseInFuseNetworkJob::Message ResponseInFuseNetworkJob::GetFreeReadPackets()
{
    return _reader.GetPackets();
}

ResponseInFuseNetworkJob::Leaf& ResponseInFuseNetworkJob::GetLeaf(std::uint64_t inode, FileTree& fileTree)
{
    return inode == FastTransport::FileSystem::RootInode ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

ResponseInFuseNetworkJob::FileHandle& ResponseInFuseNetworkJob::GetFileHandle(fuse_file_info* fileInfo)
{
    return *(reinterpret_cast<FileHandle*>(fileInfo->fh)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

std::uint64_t ResponseInFuseNetworkJob::GetINode(const Leaf& leaf)
{
    return reinterpret_cast<std::uint64_t>(&leaf); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

} // namespace FastTransport::TaskQueue
