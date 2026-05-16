#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <sys/types.h>

#include "CacheTreeJob.hpp"
#include "IPacket.hpp"

namespace FastTransport::TaskQueue {

// Applies an incoming network read block to the file cache and replies to
// any waiting FUSE requests.  Runs on the cacheTreeQueue so that mainQueue
// is free to process further incoming network responses immediately.
class ApplyBlockCacheJob : public CacheTreeJob {
    using Message = Protocol::IPacket::List;

public:
    ApplyBlockCacheJob(fuse_req_t request, fuse_ino_t inode, size_t size,
        off_t offset, off_t skipped, Message&& data);

    void ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;

    [[nodiscard]] fuse_ino_t GetInode() const noexcept override { return _inode; }

private:
    fuse_req_t _request;
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    off_t _skipped;
    Message _data;
};

} // namespace FastTransport::TaskQueue
