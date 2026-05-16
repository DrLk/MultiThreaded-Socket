#include "ResponseOpenJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "NativeFile.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[ResponseOpenJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseOpenJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    ZoneScopedN("ResponseOpenJob::ExecuteResponse");
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    fuse_file_info fileInfo {};
    reader >> request;
    reader >> inode;
    reader >> fileInfo;

    TRACER() << "Execute"
             << " request: " << request;

    const Leaf& leaf = GetLeaf(inode, fileTree);

    writer << MessageType::ResponseOpen;
    writer << request;

    auto file = std::make_unique<FileSystem::NativeFile>(leaf.GetFullPath());
    file->Open();
    if (!file->IsOpened()) {
        writer << errno;
        return {};
    }

    leaf.AddRef();

    writer << 0; // No error
    writer << fileInfo;
    // The registry owns the handle; the raw pointer goes on the wire as an
    // opaque ID. See RemoteFileHandleRegistry.hpp for the UAF this avoids.
    auto* remoteFile = fileTree.GetRemoteFileHandleRegistry().Register(
        std::make_shared<FileSystem::RemoteFileHandle>(std::move(file)));
    writer << remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
