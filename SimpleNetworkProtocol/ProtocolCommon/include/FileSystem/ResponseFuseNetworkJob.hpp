#pragma once

#include <cstdint>
#include <memory>
#include <stop_token>

#include "FileTree.hpp"
#include "IPacket.hpp"
#include "Job.hpp"
#include "Leaf.hpp"
#include "MessageReader.hpp"
#include "MessageWriter.hpp"

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
    void Accept(ITaskScheduler& scheduler, std::unique_ptr<Job>&& job) override;

protected:
    Leaf& GetLeaf(std::uint64_t inode, FileTree& fileTree);
    std::uint64_t GetINode(const Leaf& leaf);

private:
    Reader _reader;
};

} // namespace FastTransport::TaskQueue
