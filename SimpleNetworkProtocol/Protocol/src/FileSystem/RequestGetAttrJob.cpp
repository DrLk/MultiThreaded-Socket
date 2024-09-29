#include "RequestGetAttrJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestGetAttrJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestGetAttrJob::RequestGetAttrJob(fuse_req_t request, fuse_ino_t inode, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _remoteFile(remoteFile)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestGetAttrJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestGetAttr;
    writer << _request;
    writer << _inode;
    writer << _remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
