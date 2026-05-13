#pragma once

#include <cstdint>
#include <memory>
#include <stop_token>

#include "FileHandle.hpp"
#include "FileTree.hpp"
#include "Inode.hpp"
#include "Job.hpp"

namespace FastTransport::FileSystem {
struct RemoteFileHandle;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {

class CacheTreeJob : public Job {
protected:
    using FileHandle = FileSystem::FileHandle;
    using FileTree = FileSystem::FileTree;
    using Leaf = FileSystem::Leaf;

    Leaf& GetLeaf(std::uint64_t inode, FileTree& fileTree);

public:
    virtual void ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) = 0;
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    // Returns the inode this job operates on. Used by TaskScheduler to dispatch
    // same-inode jobs to the same cacheTreeQueue slot for serialization.
    [[nodiscard]] virtual std::uint64_t GetInode() const noexcept { return FastTransport::FileSystem::RootInode; }

private:
};

} // namespace FastTransport::TaskQueue
