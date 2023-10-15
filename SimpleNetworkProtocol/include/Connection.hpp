#pragma once

#include <atomic>
#include <chrono>
#include <span>
#include <stop_token>
#include <vector>

#include "ConnectionKey.hpp"
#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {
class ConnectionAddr;
} // namespace FastTransport::Protocol

namespace FastTransport::Protocol {

using namespace std::chrono_literals;
using FastTransport::Containers::LockedList;

class ISendQueue;
class IInFlightQueue;
class IRecvQueue;
class IConnectionState;

class IConnection {
public:
    using Ptr = std::shared_ptr<IConnection>;

    IConnection() = default;
    IConnection(const IConnection&) = default;
    IConnection(IConnection&&) = default;
    IConnection& operator=(const IConnection&) = default;
    IConnection& operator=(IConnection&&) = default;
    virtual ~IConnection() = default;

    [[nodiscard]] virtual bool IsConnected() const = 0;

    [[nodiscard]] virtual IPacket::List Send(std::stop_token stop, IPacket::List&& data) = 0;
    [[nodiscard]] virtual IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) = 0;

    virtual void Close() = 0;
    [[nodiscard]] virtual bool IsClosed() const = 0;
};

class Connection final : public IConnection {
    using clock = std::chrono::steady_clock;

public:
    using Ptr = std::shared_ptr<Connection>;

    Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID);
    Connection(const Connection& that) = delete;
    Connection(Connection&& that) = delete;
    Connection& operator=(const Connection& that) = delete;
    Connection& operator=(Connection&& that) = delete;
    ~Connection() override;

    [[nodiscard]] bool IsConnected() const override;
    void SetConnected(bool connected);

    [[nodiscard]] IPacket::List Send(std::stop_token stop, IPacket::List&& data) override;
    [[nodiscard]] IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) override;

    void Close() override;
    [[nodiscard]] bool IsClosed() const override;
    [[nodiscard]] bool CanBeDeleted() const;

    [[nodiscard]] IPacket::List OnRecvPackets(IPacket::Ptr&& packet);

    [[nodiscard]] const ConnectionKey& GetConnectionKey() const;
    [[nodiscard]] OutgoingPacket::List GetPacketsToSend();

    void SetInternalFreePackets(IPacket::List&& freeInternalSendPackets, IPacket::List&& freeRecvPackets);

    void ProcessSentPackets(OutgoingPacket::List&& packets);
    void ProcessRecvPackets();

    void Run();

    void SendPacket(IPacket::Ptr&& packet, bool needAck);
    void ReSendPackets(OutgoingPacket::List&& packets);

    [[nodiscard]] IPacket::List ProcessAcks();
    [[nodiscard]] OutgoingPacket::List CheckTimeouts();
    void AddAcks(std::span<SeqNumberType> acks);

    IRecvQueue& GetRecvQueue();

    [[nodiscard]] IPacket::List GetFreeRecvPackets();

    [[nodiscard]] IPacket::List CleanFreeRecvPackets();
    [[nodiscard]] IPacket::List CleanFreeSendPackets();

    void AddFreeUserSendPackets(IPacket::List&& freePackets);

    IConnectionState* _state; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    LockedList<IPacket::Ptr> _sendUserData; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    ConnectionKey _key; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    ConnectionID _destinationID { 0 }; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    LockedList<IPacket::Ptr> _freeRecvPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    IPacket::List _freeInternalSendPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    LockedList<std::vector<char>> _recvUserData;

    clock::time_point _lastPacketReceive;

    std::unique_ptr<IInFlightQueue> _inFlightQueue;
    std::unique_ptr<IRecvQueue> _recvQueue;
    std::unique_ptr<ISendQueue> _sendQueue;

    LockedList<IPacket::Ptr> _freeUserSendPackets;

    std::atomic<bool> _connected { false };
    std::atomic<bool> _closed;
    std::atomic<bool> _cleanRecvBuffers;
    std::atomic<bool> _cleanSendBuffers;
};
} // namespace FastTransport::Protocol
