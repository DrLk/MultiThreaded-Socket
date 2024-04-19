#include "ResponseReadDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReadDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t off = 0;
    int file = 0;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> off;
    reader >> file;

    TRACER() << "Execute"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " off: " << off
             << " file: " << file;

    writer << MessageType::ResponseReadDir;
    writer << request;

    int error = 0;
    writer << error;

    return {};
}

} // namespace FastTransport::TaskQueue
