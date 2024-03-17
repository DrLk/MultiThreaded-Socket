#pragma once

#include "FileTree.hpp"
#include "ReadNetworkJob.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[MessageTypeReadJob] "

namespace FastTransport::TaskQueue {

class MessageTypeReadJob : public ReadNetworkJob {
    using FileTree = FastTransport::FileSystem::FileTree;
public:
    static std::unique_ptr<MessageTypeReadJob> Create(FileTree& fileTree, Message&& messages);

    MessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages);

    void ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection);

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    Message _messages;
};

} // namespace FastTransport::TaskQueue
