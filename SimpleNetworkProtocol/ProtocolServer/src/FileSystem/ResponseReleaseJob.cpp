#include "ResponseReleaseJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "FileHandle.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[ResponseReleaseJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReleaseJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    ZoneScopedN("ResponseReleaseJob::ExecuteResponse");
    TRACER() << "Execute";

    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    const FileSystem::FileHandle* handle = nullptr;
    reader >> request;
    reader >> inode;
    reader >> remoteFile;
    reader >> handle;

    writer << MessageType::ResponseRelease;
    writer << request;

    const int error = 0;

    auto& leaf = GetLeaf(inode, fileTree);
    leaf.ReleaseRef();

    if (inode == FUSE_ROOT_ID) {
        assert(false);
        writer << error;
        writer << handle;
        return {};
    }

    // Owning RemoteFileHandle was created by ResponseOpenJob; closing it here
    // releases the underlying file descriptor through ~NativeFile.
    delete remoteFile; // NOLINT(cppcoreguidelines-owning-memory)

    writer << error;
    writer << handle;

    return {};
}

} // namespace FastTransport::TaskQueue
