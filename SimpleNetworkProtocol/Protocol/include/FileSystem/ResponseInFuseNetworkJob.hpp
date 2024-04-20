#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>

#include "FileHandle.hpp"
#include "FileTree.hpp"
#include "IPacket.hpp"
#include "Job.hpp"
#include "Leaf.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {

class ResponseInFuseNetworkJob : public Job {
protected:
    using Message = Protocol::IPacket::List;
    using Reader = Protocol::MessageReader;
    using FileTree = FileSystem::FileTree;
    using Leaf = FileSystem::Leaf;
    using FileHandle = FileSystem::FileHandle;

public:
    virtual Message ExecuteResponse(std::stop_token stop, FileTree& fileTree) = 0;

    void InitReader(Reader&& reader);
    Reader& GetReader();
    Message GetFreeReadPackets();

protected:
    Leaf& GetLeaf(fuse_ino_t inode, FileTree& fileTree);
    FileHandle& GetFileHandle(fuse_file_info* fileInfo);
    fuse_ino_t GetINode(const Leaf& leaf);

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    Reader _reader;
};

} // namespace FastTransport::TaskQueue
