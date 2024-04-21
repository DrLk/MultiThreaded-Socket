#include "ResponseGetAttrJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseGetAttrJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseGetAttrJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    reader >> request;
    reader >> inode;
    reader >> remoteFile;

    writer << MessageType::ResponseGetAttr;
    writer << request;

    int error = 0;

    if (inode == FUSE_ROOT_ID) {
        writer << error;
        writer << inode;
        writer << (S_IFDIR | 0755);
        writer << 2;
        writer << 0;
        return {};
    }

    struct stat stbuf { };
    const int file = remoteFile != nullptr ? remoteFile->file : 0;
    if (file == 0) {
        error = stat(GetLeaf(inode, fileTree).GetFullPath().c_str(), &stbuf);
    } else {
        error = fstatat(file, "", &stbuf, 0);
    }

    writer << error;

    if (error == 0) {
        writer << inode;
        writer << stbuf.st_mode;
        writer << stbuf.st_nlink;
        writer << stbuf.st_size;
        writer << stbuf.st_uid;
        writer << stbuf.st_gid;
        writer << stbuf.st_mtim;
    }

    return {};
}

} // namespace FastTransport::TaskQueue
