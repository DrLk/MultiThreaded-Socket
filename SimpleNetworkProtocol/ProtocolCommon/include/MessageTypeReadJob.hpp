#pragma once

#include "FileTree.hpp"
#include "MessageType.hpp"
#include "ReadNetworkJob.hpp"

namespace FastTransport::Protocol {
class MessageReader;
} // namespace FastTransport::Protocol

namespace FastTransport::TaskQueue {

// Base dispatcher. Reads message-type tags off the network and either
// constructs a side-agnostic handler (RequestTree/ResponseTree) directly, or
// delegates side-specific (Request* / Response* / Notify*) types to
// DispatchSideSpecific, which the per-side subclasses override. The static
// Create method is intentionally absent on this class — main_server.cpp uses
// ServerMessageTypeReadJob::Create, main_client.cpp uses
// ClientMessageTypeReadJob::Create.
class MessageTypeReadJob : public ReadNetworkJob {
protected:
    using FileTree = FastTransport::FileSystem::FileTree;

public:
    MessageTypeReadJob(FastTransport::FileSystem::FileTree& fileTree, Message&& messages);

    void ExecuteReadNetwork(std::stop_token stop, ITaskScheduler& scheduler, IConnection& connection) override;

protected:
    [[nodiscard]] FastTransport::FileSystem::FileTree& GetFileTree() noexcept { return _fileTree; }
    // Subclass returns a fresh ReadNetworkJob that continues reading from the
    // same connection. Used to re-arm the read loop after the current message
    // is dispatched.
    [[nodiscard]] virtual std::unique_ptr<ReadNetworkJob> CreateNext(Message&& messages) = 0;
    // Handle a side-specific message. Returns true if handled, false if the
    // type is unknown for this side (caller will throw).
    virtual bool DispatchSideSpecific(MessageType type, Protocol::MessageReader&& reader, ITaskScheduler& scheduler) = 0;

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    Message _messages;
};

} // namespace FastTransport::TaskQueue
