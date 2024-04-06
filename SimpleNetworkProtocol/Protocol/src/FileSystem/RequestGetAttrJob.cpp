#include "RequestGetAttrJob.hpp"

#include "FuseReadFileJob.hpp"
#include <fuse3/fuse_lowlevel.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestGetAttrJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestGetAttrJob::RequestGetAttrJob(fuse_req_t request, fuse_ino_t inode, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _fileInfo(fileInfo)
{
    TRACER() << "Create";
}

Message RequestGetAttrJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute";

    writer << MessageType::RequestGetAttr;
    writer << _request;
    writer << _inode;
    writer << _fileInfo;

    return {};
}

} // namespace FastTransport::TaskQueue
