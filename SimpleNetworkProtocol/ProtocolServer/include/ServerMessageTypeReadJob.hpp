#pragma once

#include <memory>

#include "MessageTypeReadJob.hpp"

namespace FastTransport::TaskQueue {

class ServerMessageTypeReadJob final : public MessageTypeReadJob {
public:
    ServerMessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages)
        : MessageTypeReadJob(fileTree, std::move(messages))
    {
    }

    static std::unique_ptr<ReadNetworkJob> Create(FileTree& fileTree, Message&& messages);

protected:
    [[nodiscard]] std::unique_ptr<ReadNetworkJob> CreateNext(Message&& messages) override;
    bool DispatchSideSpecific(MessageType type, Protocol::MessageReader&& reader, ITaskScheduler& scheduler) override;
};

} // namespace FastTransport::TaskQueue
