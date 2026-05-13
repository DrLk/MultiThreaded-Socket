#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <memory>

#include "CacheTreeJob.hpp"

namespace FastTransport::FileSystem {
class Leaf;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {

// Clears a Leaf's block cache and notifies the FUSE kernel to evict its page cache.
// Runs on the _cacheTreeQueue (same thread as all other Leaf data operations) so no
// mutex is needed on Leaf::_data.
//
// Lifetime: holds the Leaf alive via a shared_ptr for the duration of the job —
// the dispatch hop from _mainQueue to the cacheTreeQueue shard cannot let the
// Leaf disappear, regardless of whether RemoveChild on _mainQueue drops the
// parent's shared_ptr in between. This avoids re-querying _serverInodeIndex
// (which would race with _mainQueue mutations) and avoids touching nlookup
// (which would force _selfPin into multi-threaded territory).
class InvalidateCacheLeafJob : public CacheTreeJob {
public:
    InvalidateCacheLeafJob(std::shared_ptr<FileSystem::Leaf> leaf, fuse_ino_t clientInode, fuse_session* session);

    void ExecuteCachedTree(ITaskScheduler& scheduler, std::stop_token stop, FileTree& tree) override;
    [[nodiscard]] fuse_ino_t GetInode() const noexcept override { return _clientInode; }

private:
    std::shared_ptr<FileSystem::Leaf> _leaf;
    fuse_ino_t _clientInode;
    fuse_session* _session;
};

} // namespace FastTransport::TaskQueue
