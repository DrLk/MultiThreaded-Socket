#include "ResponseReadDirPlusJob.hpp"

#include <fuse3/fuse_lowlevel.h>
#include <stop_token>
#include <sys/stat.h>

#include "DirectoryEntryWriter.hpp"
#include "Logger.hpp"
#include "MessageType.hpp"
#include "RemoteFileHandle.hpp"
#include "ResponseFuseNetworkJob.hpp"

#define TRACER() LOGGER() << "[ResponseReadDirPlusJob] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::TaskQueue {

ResponseFuseNetworkJob::Message ResponseReadDirPlusJob::ExecuteResponse(std::stop_token /*stop*/, Writer& writer, FileTree& fileTree)
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

    writer << MessageType::ResponseReadDirPlus;
    writer << request;

    Protocol::DirectoryEntryWriter direcotoryWriter(writer.GetDataPackets(99));

    auto& parent = GetLeaf(inode, fileTree);
    if (offset < 1) {
        struct stat stbuf { };
        memset(&stbuf, 0, sizeof(stbuf));
        stat(parent.GetFullPath().c_str(), &stbuf);
        stbuf.st_ino = inode;
        direcotoryWriter.AddDirectoryEntryPlus(".", &stbuf, 0);
    }

    if (offset < 2) {
        struct stat stbuf { };
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = 1;
        direcotoryWriter.AddDirectoryEntryPlus("..", &stbuf, 1);
    }

    const auto& children = parent.GetChildren();

    auto child = children.begin();
    for (off_t i = 2; i <= offset && child != children.end(); ++i, ++child) {
    }

    for (off_t i = 0; i < size && child != children.end(); ++i, ++child) {
        struct stat stbuf { };
        stat(child->second.GetFullPath().c_str(), &stbuf);
        stbuf.st_ino = GetINode(child->second);
        direcotoryWriter.AddDirectoryEntryPlus(child->first, &stbuf, i + 2);
    }

    writer << direcotoryWriter.GetWritedPackets();
    writer.AddFreePackets(direcotoryWriter.GetPackets());

    return {};
}

} // namespace FastTransport::TaskQueue
