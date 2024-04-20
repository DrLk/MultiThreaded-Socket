#include "RequestReadDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReadDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReadDirJob::RequestReadDirJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t off, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _off(off)
    , _fileInfo(fileInfo)
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
    const int file = _fileInfo != nullptr ? GetFileHandle(_fileInfo).remoteFile.file : 0;
    writer << file;

    return {};
}

} // namespace FastTransport::TaskQueue
