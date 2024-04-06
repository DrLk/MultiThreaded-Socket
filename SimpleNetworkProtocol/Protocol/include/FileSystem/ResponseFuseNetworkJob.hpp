#pragma once

#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <stop_token>

#include "FileTree.hpp"
#include "IPacket.hpp"
#include "Job.hpp"
#include "MessageReader.hpp"
#include "MessageWriter.hpp"

namespace FastTransport::FileSystem {
class Leaf;
} // namespace FastTransport::FileSystem

namespace FastTransport::TaskQueue {

class ResponseFuseNetworkJob : public Job {
protected:
    using Message = Protocol::IPacket::List;
    using Writer = Protocol::MessageWriter;
    using Reader = Protocol::MessageReader;
    using FileTree = FileSystem::FileTree;
    using Leaf = FileSystem::Leaf;

public:
    virtual Message ExecuteResponse(std::stop_token stop, Writer& writer, FileTree& fileTree) = 0;

    void InitReader(Reader&& reader);
    Reader& GetReader();
    Message GetFreeReadPackets();

protected:
    Leaf& GetLeaf(fuse_ino_t inode, FileTree& fileTree);
    fuse_ino_t GetINode(const Leaf& leaf);

private:
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

    Reader _reader;
};

} // namespace FastTransport::TaskQueue
