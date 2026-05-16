#include "ReleaseLeafRefJob.hpp"

#include <stop_token>

#include "Leaf.hpp"

namespace FastTransport::TaskQueue {

ReleaseLeafRefJob::ReleaseLeafRefJob(fuse_ino_t clientInode, std::uint64_t nlookup)
    : _clientInode(clientInode)
    , _nlookup(nlookup)
{
}

ResponseInFuseNetworkJob::Message ReleaseLeafRefJob::ExecuteResponse(ITaskScheduler& /*scheduler*/, std::stop_token /*stop*/, FileTree& fileTree)
{
    auto& leaf = GetLeaf(_clientInode, fileTree);
    leaf.ReleaseRef(_nlookup);
    return {};
}

} // namespace FastTransport::TaskQueue
