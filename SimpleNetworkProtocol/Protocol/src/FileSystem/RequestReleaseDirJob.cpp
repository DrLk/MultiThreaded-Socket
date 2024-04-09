#include "RequestReleaseDirJob.hpp"

#include "FuseReadFileJob.hpp"
#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReleaseDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReleaseDirJob::RequestReleaseDirJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _file(static_cast<int>(fileInfo->fh))
{
    TRACER() << "Create";
}

Message RequestReleaseDirJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestReleaseDir;
    writer << _request;
    writer << _inode;
    writer << _file;

    return {};
}

} // namespace FastTransport::TaskQueue
