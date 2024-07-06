#include "CacheTreeJob.hpp"

#include "ITaskScheduler.hpp"

namespace FastTransport::TaskQueue {

CacheTreeJob::Leaf& CacheTreeJob::GetLeaf(fuse_ino_t inode, FileTree& fileTree)
{
    return inode == FUSE_ROOT_ID ? fileTree.GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
}

void CacheTreeJob::Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job)
{
    auto* pointer = dynamic_cast<CacheTreeJob*>(job.release());
    std::unique_ptr<CacheTreeJob> cacheTreeJob(pointer);
    scheduler.ScheduleCacheTreeJob(std::move(cacheTreeJob));
}

} // namespace FastTransport::TaskQueue
