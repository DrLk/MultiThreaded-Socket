#include "RequestReleaseJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReleaseJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReleaseJob::RequestReleaseJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _handle(&GetFileHandle(fileInfo))
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestReleaseJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestRelease;
    writer << _request;
    writer << _inode;
    writer << _handle->remoteFile.file;
    writer << _handle;

    return {};
}

} // namespace FastTransport::TaskQueue
