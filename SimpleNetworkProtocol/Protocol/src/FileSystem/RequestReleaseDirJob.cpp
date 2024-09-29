#include "RequestReleaseDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReleaseDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReleaseDirJob::RequestReleaseDirJob(fuse_req_t request, fuse_ino_t inode, FileHandle* handle)
    : _request(request)
    , _inode(inode)
    , _handle(handle)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestReleaseDirJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestReleaseDir;
    writer << _request;
    writer << _inode;
    writer << _handle->remoteFile;
    writer << _handle;

    return {};
}

} // namespace FastTransport::TaskQueue
