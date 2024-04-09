#include "ResponseReleaseDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReleaseDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    int file = 0;
    reader >> request;
    reader >> inode;
    reader >> file;

    writer << MessageType::ResponseReleaseDir;
    writer << request;

    int error = 0;

    auto& leaf = GetLeaf(inode, fileTree);
    leaf.ReleaseRef();

    if (inode == FUSE_ROOT_ID) {
        writer << error;
        return {};
    }

    if (close(file) == -1) {
        error = errno;
    }

    writer << error;

    return {};
}

} // namespace FastTransport::TaskQueue
