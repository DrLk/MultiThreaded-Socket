#include "InvalidateCacheLeafJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileTree.hpp"
#include "ITaskScheduler.hpp"
#include "Leaf.hpp"

namespace FastTransport::TaskQueue {

InvalidateCacheLeafJob::InvalidateCacheLeafJob(fuse_ino_t clientInode, fuse_ino_t serverInode, fuse_session* session)
    : _clientInode(clientInode)
    , _serverInode(serverInode)
    , _session(session)
{
}

void InvalidateCacheLeafJob::ExecuteCachedTree(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& tree)
{
    FileSystem::Leaf* leaf = tree.FindLeafByServerInode(_serverInode);
    if (leaf == nullptr) {
        return;
    }
    leaf->InvalidateDataCache();
    if (_session != nullptr) {
        fuse_lowlevel_notify_inval_inode(_session, _clientInode, 0, 0);
    }
}

} // namespace FastTransport::TaskQueue
