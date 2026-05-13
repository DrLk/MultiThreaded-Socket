#include "CacheTreeJob.hpp"

#include <cstdint>

#include "IClientTaskScheduler.hpp"
#include "ITaskScheduler.hpp"
#include "Inode.hpp"

namespace FastTransport::TaskQueue {

CacheTreeJob::Leaf& CacheTreeJob::GetLeaf(std::uint64_t inode, FileTree& fileTree)
{
    return inode == FastTransport::FileSystem::RootInode ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

void CacheTreeJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job)
{
    auto* pointer = dynamic_cast<CacheTreeJob*>(std::move(job).release());
    std::unique_ptr<CacheTreeJob> cacheTreeJob(pointer);
    dynamic_cast<IClientTaskScheduler&>(scheduler).ScheduleCacheTreeJob(std::move(cacheTreeJob));
}

} // namespace FastTransport::TaskQueue
