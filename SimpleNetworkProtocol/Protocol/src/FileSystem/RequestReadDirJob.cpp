#include "RequestReadDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[RequestReadDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReadDirJob::RequestReadDirJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t off, FileSystem::RemoteFileHandle* remoteFile)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _off(off)
    , _remoteFile(remoteFile)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestReadDirJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestReadDir;
    writer << _request;
    writer << _inode;
    writer << _size;
    writer << _off;
    writer << _remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
