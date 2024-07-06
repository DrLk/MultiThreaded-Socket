#include "RequestReadFileJob.hpp"

#include <fuse3/fuse_lowlevel.h>

#include "ITaskScheduler.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[RequestReadFileJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

RequestReadFileJob::RequestReadFileJob(fuse_req_t request, fuse_ino_t inode, size_t size, off_t offset, fuse_file_info* fileInfo)
    : _request(request)
    , _inode(inode)
    , _size(size)
    , _offset(offset)
    , _fileInfo(fileInfo)
{
}

FuseNetworkJob::Message RequestReadFileJob::ExecuteMain(std::stop_token  /*stop*/, Writer& writer)
{
    TRACER() << "Execute"
             << "request: " << _request
             << " inode: " << _inode
             << " size: " << _size
             << " offset: " << _offset
             << " file: " << _fileInfo;


    writer << MessageType::RequestRead;
    writer << _request;
    writer << _inode;
    writer << _size;
    writer << _offset;
    writer << GetFileHandle(_fileInfo).remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
