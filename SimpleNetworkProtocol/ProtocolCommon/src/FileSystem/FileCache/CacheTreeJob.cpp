#include "CacheTreeJob.hpp"

#include <cstdint>

#include "ITaskScheduler.hpp"
#include "Inode.hpp"

namespace FastTransport::TaskQueue {

CacheTreeJob::Leaf& CacheTreeJob::GetLeaf(std::uint64_t inode, FileTree& fileTree)
{
    return inode == FastTransport::FileSystem::RootInode ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

void CacheTreeJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    // See FuseNetworkJob::Accept for rationale on static_cast over dynamic_cast.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    std::unique_ptr<CacheTreeJob> cacheTreeJob(static_cast<CacheTreeJob*>(job.release()));
    scheduler.ScheduleCacheTreeJob(std::move(cacheTreeJob));
}

} // namespace FastTransport::TaskQueue
