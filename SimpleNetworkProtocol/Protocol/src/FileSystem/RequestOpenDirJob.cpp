#include "RequestOpenDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestOpenDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestOpenDirJob::RequestOpenDirJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _fileInfo(fileInfo)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestOpenDirJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request
             << " inode: " << _inode
             << " fileInfo: " << _fileInfo;

    writer << MessageType::RequestOpenDir;
    writer << _request;
    writer << _inode;
    writer << _fileInfo;
    writer << _fileInfo->flags;

    return {};
}

} // namespace FastTransport::TaskQueue
