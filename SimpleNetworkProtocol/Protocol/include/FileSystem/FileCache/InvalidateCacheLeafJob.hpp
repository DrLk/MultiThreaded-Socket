#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "CacheTreeJob.hpp"

namespace FastTransport::TaskQueue {

// Clears a Leaf's block cache and notifies the FUSE kernel to evict its page cache.
// Runs on the _cacheTreeQueue (same thread as all other Leaf data operations) so no
// mutex is needed on Leaf::_data.
class InvalidateCacheLeafJob : public CacheTreeJob {
public:
    InvalidateCacheLeafJob(fuse_ino_t clientInode, fuse_ino_t serverInode, fuse_session* session);

    void ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;
    [[nodiscard]] fuse_ino_t GetInode() const noexcept override { return _clientInode; }

private:
    fuse_ino_t _clientInode;
    fuse_ino_t _serverInode;
    fuse_session* _session;
};

} // namespace FastTransport::TaskQueue
