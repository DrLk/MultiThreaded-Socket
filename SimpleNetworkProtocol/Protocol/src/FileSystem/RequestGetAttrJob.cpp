#include "RequestGetAttrJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
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

FuseNetworkJob::Message RequestGetAttrJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestGetAttr;
    writer << _request;
    writer << _inode;
    const int file = _fileInfo != nullptr ? GetFileHandle(_fileInfo).remoteFile.file : 0;
    writer << file;

    return {};
}

} // namespace FastTransport::TaskQueue
