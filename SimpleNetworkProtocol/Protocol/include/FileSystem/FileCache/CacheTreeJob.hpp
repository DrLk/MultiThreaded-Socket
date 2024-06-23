#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>

#include "FileHandle.hpp"
#include "FileTree.hpp"
#include "Job.hpp"

namespace FastTransport::TaskQueue {

class CacheTreeJob : public Job {
protected:
    using FileHandle = FileSystem::FileHandle;
    using FileTree = FileSystem::FileTree;
    using Leaf = FileSystem::Leaf;

    Leaf& GetLeaf(fuse_ino_t inode, FileTree& fileTree);

public:
    virtual void ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) = 0;


private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;
};

} // namespace FastTransport::TaskQueue
