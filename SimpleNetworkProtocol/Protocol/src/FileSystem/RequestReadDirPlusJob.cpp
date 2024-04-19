#include "RequestReadDirPlusJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "FuseNetworkJob.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReadDirPlusJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReadDirPlusJob::RequestReadDirPlusJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t off, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _off(off)
    , _fileInfo(fileInfo)
{
    TRACER() << "Create";
}

FuseNetworkJob::Message RequestReadDirPlusJob::ExecuteMain(std::stop_token /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << " request: " << _request;

    writer << MessageType::RequestReadDirPlus;
    writer << _request;
    writer << _inode;
    writer << _size;
    writer << _off;
    const int file = _fileInfo != nullptr ? static_cast<int>(_fileInfo->fh) : 0;
    writer << file;

    return {};
}

} // namespace FastTransport::TaskQueue
