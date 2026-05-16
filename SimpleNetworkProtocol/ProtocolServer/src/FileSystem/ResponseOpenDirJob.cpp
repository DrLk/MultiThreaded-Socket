#include "ResponseOpenDirJob.hpp"
#include <Tracy.hpp>

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>
#include <utility>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "NativeFile.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[ResponseOpenDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseOpenDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    ZoneScopedN("ResponseOpenDirJob::ExecuteResponse");
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
    leaf.AddRef();

    writer << MessageType::ResponseOpenDir;
    writer << request;

    auto file = std::make_unique<FileSystem::NativeFile>(leaf.GetFullPath());
    file->Open();
    if (!file->IsOpened()) {
        writer << errno;
        return {};
    }

    writer << 0; // No error
    writer << fileInfo;
    auto* remoteFile = fileTree.GetRemoteFileHandleRegistry().Register(
        std::make_shared<FileSystem::RemoteFileHandle>(std::move(file)));
    writer << remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
