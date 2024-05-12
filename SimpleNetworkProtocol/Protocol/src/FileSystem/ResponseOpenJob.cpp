#include "ResponseOpenJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "Logger.hpp"
#include "MessageType.hpp"
#include "NativeFile.hpp"
#include "RemoteFileHandle.hpp"

#define TRACER() LOGGER() << "[ResponseOpenJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseOpenJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
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

    writer << MessageType::ResponseOpen;
    writer << request;

    auto file = std::make_unique<FileSystem::NativeFile>(leaf.GetFullPath());
    if (!file->IsOpened()) {
        writer << errno;
        return {};
    }

    leaf.AddRef();

    writer << 0; // No error
    writer << fileInfo;
    auto* remoteFile = new FileSystem::RemoteFileHandle { std::move(file) }; // NOLINT(cppcoreguidelines-owning-memory)
    writer << remoteFile;

    return {};
}

} // namespace FastTransport::TaskQueue
