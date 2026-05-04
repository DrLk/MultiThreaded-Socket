#pragma once

#include "CacheTreeJob.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileCache {
class WriteFileCacheJob : public TaskQueue::CacheTreeJob {
public:
    using Message = Protocol::IPacket::List;

    WriteFileCacheJob(fuse_ino_t inode, size_t size, off_t offset, Message&& data);
    void ExecuteCachedTree(TaskQueue::ITaskScheduler& scheduler, std::stop_token stop, FileTree& fileTree) override;

    [[nodiscard]] fuse_ino_t GetInode() const noexcept override { return _inode; }

private:
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    Message _data;
};
} // namespace FastTransport::FileCache
