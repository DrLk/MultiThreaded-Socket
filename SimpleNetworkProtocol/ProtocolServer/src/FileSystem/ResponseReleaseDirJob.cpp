#include "ResponseReleaseDirJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileHandle.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "RemoteFileHandleRegistry.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReleaseDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    ZoneScopedN("ResponseReleaseDirJob::ExecuteResponse");
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    const FileSystem::FileHandle* fileHandle = nullptr;
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

    // Take the registry's owning shared_ptr. Even though dir handles are
    // never used off the main queue today, going through the registry keeps
    // ownership symmetric with file handles and avoids a double-free if a
    // disk path is ever added for directories.
    auto owner = RemoteFileHandleRegistry::Instance().Take(remoteFile);

    if (inode == FUSE_ROOT_ID) {
        writer << error;
        writer << fileHandle;
        return {};
    }

    if (remoteFile->file2->Close() == -1) {
        error = errno;
        writer << error;
        return {};
    }

    writer << error;
    writer << fileHandle;

    return {};
}

} // namespace FastTransport::TaskQueue
