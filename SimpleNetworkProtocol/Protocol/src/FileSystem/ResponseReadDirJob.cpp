#include "ResponseReadDirJob.hpp"

#include <cstring>
#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "DirectoryEntryWriter.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

namespace {
    void AddFile(Protocol::DirectoryEntryWriter& writer, std::string_view name, fuse_ino_t ino, off_t offset)
    {
        struct stat stbuf { };
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = ino;
        writer.AddDirectoryEntry(name, &stbuf, offset);
    }

} // namespace

ResponseFuseNetworkJob::Message ResponseReadDirJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
{
    auto& reader = GetReader();
    fuse_req_t request = nullptr;
    fuse_ino_t inode = 0;
    size_t size = 0;
    off_t offset = 0;
    FileSystem::RemoteFileHandle* remoteFile = nullptr;
    reader >> request;
    reader >> inode;
    reader >> size;
    reader >> offset;
    reader >> remoteFile;

    TRACER() << "Execute"
             << " request: " << request
             << " inode: " << inode
             << " size: " << size
             << " offset: " << offset
             << " file: " << remoteFile;

    writer << MessageType::ResponseReadDir;
    writer << request;

    Protocol::DirectoryEntryWriter direcotoryWriter(writer.GetDataPackets(99));

    if (offset < 1) {
        AddFile(direcotoryWriter, ".", inode, 0);
    }

    if (offset < 2) {
        AddFile(direcotoryWriter, "..", 1, 1);
    }

    auto& parent = GetLeaf(inode, fileTree);
    const auto& children = parent.GetChildren();

    auto child = children.begin();
    for (off_t i = 2; i <= offset && child != children.end(); ++i, ++child) {
    }

    for (off_t i = 0; i < size && child != children.end(); ++i, ++child) {
        AddFile(direcotoryWriter, child->first, GetINode(child->second), i + 2);
    }

    writer << direcotoryWriter.GetWritedPackets();
    writer.AddFreePackets(direcotoryWriter.GetPackets());

    return {};
}

} // namespace FastTransport::TaskQueue
