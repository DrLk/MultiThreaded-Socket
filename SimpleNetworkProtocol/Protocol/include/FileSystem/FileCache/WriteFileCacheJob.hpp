#pragma once

#include "ResponseInFuseNetworkJob.hpp"

namespace FastTransport::FileCache {
class WriteFileCacheJob : public TaskQueue::ResponseInFuseNetworkJob {
public:
    using Message = Protocol::IPacket::List;

    WriteFileCacheJob(fuse_ino_t inode, size_t size, off_t offset, Message&& data, FileSystem::RemoteFileHandle* remoteFile);
    Message ExecuteResponse(std::stop_token stop, FileTree& fileTree) override;

private:
    fuse_ino_t _inode;
    size_t _size;
    off_t _offset;
    Message _data;
    FileSystem::RemoteFileHandle* _remoteFile;
};
} // namespace FastTransport::FileCache
