#include "RequestReleaseJob.hpp"

#include "FuseReadFileJob.hpp"
#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestGetAttrJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReleaseJob::RequestReleaseJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _file(static_cast<int>(fileInfo->fh))
{
    TRACER() << "Create";
}

Message RequestReleaseJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestRelease;
    writer << _request;
    writer << _inode;
    writer << _file;

    return {};
}

} // namespace FastTransport::TaskQueue
