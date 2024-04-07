#include "ResponseOpenDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"

#define TRACER() LOGGER() << "[ResponseOpendDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseOpenDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    fuse_file_info* fileInfo = nullptr;
    int flags = 0;
    reader >> request;
    reader >> inode;
    reader >> fileInfo;
    reader >> flags;

    TRACER() << "Execute"
             << " request: " << request;

    Leaf const& leaf = GetLeaf(inode, fileTree);

    writer << MessageType::ResponseOpenDir;
    writer << request;

    const int file = open(leaf.GetFullPath().c_str(), flags & ~O_NOFOLLOW); // NOLINT(hicpp-signed-bitwise, hicpp-vararg, cppcoreguidelines-pro-type-vararg)
    if (file == -1) {
        writer << errno;
        return {};
    }

    writer << 0; // No error
    writer << fileInfo;
    writer << file;

    return {};
}

} // namespace FastTransport::TaskQueue
