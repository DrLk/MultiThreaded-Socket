#pragma once

#include <fuse3/fuse_lowlevel.h>

#include "ResponseInFuseNetworkJob.hpp"

namespace FastTransport::TaskQueue {

// Decrements nlookup on the client-side Leaf identified by `clientInode`
// (= reinterpret_cast<fuse_ino_t>(Leaf*)). Scheduled by RemoteFileSystem's
// FUSE forget callback so the actual ReleaseRef call runs on _mainQueue
// rather than on the FUSE worker thread, preserving the single-thread
// invariant on Leaf::_nlookup / Leaf::_selfPin.
class ReleaseLeafRefJob : public ResponseInFuseNetworkJob {
public:
    ReleaseLeafRefJob(fuse_ino_t clientInode, std::uint64_t nlookup);

    Message ExecuteResponse(ITaskScheduler& scheduler, std::stop_token stop, FileTree& fileTree) override;

private:
    fuse_ino_t _clientInode;
    std::uint64_t _nlookup;
};

} // namespace FastTransport::TaskQueue
