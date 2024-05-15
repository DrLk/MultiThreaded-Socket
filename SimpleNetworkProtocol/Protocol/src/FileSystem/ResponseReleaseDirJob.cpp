#include "ResponseReleaseDirJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileHandle.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReleaseDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    FileSystem::FileHandle* fileHandle = nullptr;
    reader >> request;
    reader >> inode;
    reader >> remoteFile;
    ;
    reader >> fileHandle;

    writer << MessageType::ResponseReleaseDir;
    writer << request;

    int error = 0;

    auto& leaf = GetLeaf(inode, fileTree);
    leaf.ReleaseRef();

    if (inode == FUSE_ROOT_ID) {
        delete fileHandle->remoteFile; // NOLINT(cppcoreguidelines-owning-memory)
        writer << error;
        writer << fileHandle;
        return {};
    }

    if (remoteFile->file2->Close() == -1) {
        error = errno;
        writer << error;
        return {};
    }

    delete fileHandle->remoteFile; // NOLINT(cppcoreguidelines-owning-memory)
    writer << error;
    writer << fileHandle;

    return {};
}

} // namespace FastTransport::TaskQueue
