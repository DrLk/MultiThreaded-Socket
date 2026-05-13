#pragma once

#include <memory>

#include "MessageTypeReadJob.hpp"

namespace FastTransport::TaskQueue {

class ClientMessageTypeReadJob final : public MessageTypeReadJob {
public:
    using MessageTypeReadJob::MessageTypeReadJob;

    static std::unique_ptr<ReadNetworkJob> Create(FileTree& fileTree, Message&& messages);

protected:
    [[nodiscard]] std::unique_ptr<ReadNetworkJob> CreateNext(Message&& messages) override;
    bool DispatchSideSpecific(MessageType type, Protocol::MessageReader&& reader, ITaskScheduler& scheduler) override;
};

} // namespace FastTransport::TaskQueue
