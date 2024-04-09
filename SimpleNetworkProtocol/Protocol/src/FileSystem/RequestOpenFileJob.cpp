#include "RequestOpenJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestOpenJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestOpenJob::RequestOpenJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _fileInfo(fileInfo)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestOpenJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request
             << " inode: " << _inode
             << " fileInfo: " << _fileInfo;

    writer << MessageType::RequestOpen;
    writer << _request;
    writer << _inode;
    writer << _fileInfo;
    writer << _fileInfo->flags;

    return {};
}

} // namespace FastTransport::TaskQueue
