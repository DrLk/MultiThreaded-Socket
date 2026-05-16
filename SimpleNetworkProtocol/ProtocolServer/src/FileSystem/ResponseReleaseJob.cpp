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

    // Take the registry's owning shared_ptr; if an in-flight Read on the
    // disk thread still holds an Acquire'd copy, ~RemoteFileHandle is
    // deferred until that copy drops. Dropping `owner` here is the normal
    // path that closes the underlying file descriptor via ~NativeFile.
    auto owner = fileTree.GetRemoteFileHandleRegistry().Take(remoteFile);

    writer << error;
    writer << handle;

    return {};
}

} // namespace FastTransport::TaskQueue
