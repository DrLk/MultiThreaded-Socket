#include "ResponseReadDirPlusJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReadDirPlusJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& /*fileTree*/)
{

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> remoteFile;

    TRACER() << "Execute"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " offset: " << offset
             << " file: " << remoteFile;

    writer << MessageType::ResponseReadDirPlus;
    writer << request;

    const int error = 0;
    writer << error;

    return {};
}

} // namespace FastTransport::TaskQueue
