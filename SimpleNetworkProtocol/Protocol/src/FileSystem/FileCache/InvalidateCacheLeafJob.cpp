#include "InvalidateCacheLeafJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"

namespace FastTransport::TaskQueue {

InvalidateCacheLeafJob::InvalidateCacheLeafJob(std::shared_ptr<FileSystem::Leaf> leaf, fuse_ino_t clientInode, fuse_session* session)
    : _leaf(std::move(leaf))
    , _clientInode(clientInode)
    , _session(session)
{
}

void InvalidateCacheLeafJob::ExecuteCachedTree(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& /*tree*/)
{
    // _leaf shared_ptr keeps the Leaf alive for the duration of this job,
    // so RemoveChild on _mainQueue dropping the parent's shared_ptr in the
    // meantime cannot free us. Touch only the data-cache state which is
    // shard-affine on this cacheTreeQueue worker.
    _leaf->InvalidateDataCache();
    if (_session != nullptr) {
        fuse_lowlevel_notify_inval_inode(_session, _clientInode, 0, 0);
    }
}

} // namespace FastTransport::TaskQueue
