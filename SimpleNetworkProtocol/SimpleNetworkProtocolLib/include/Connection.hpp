#pragma once

#include <atomic>
#include <chrono>
#include <span>
#include <stop_token>
#include <unordered_map>
#include <vector>

#include "ConnectionContext.hpp"
#include "ConnectionKey.hpp"
#include "ConnectionState.hpp"
#include "HeaderTypes.hpp"
#include "IConnectionInternal.hpp"
#include "IPacket.hpp"
#include "IStatistics.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"
#include "Statistics.hpp"

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

class Connection final : public IConnectionInternal {
    using clock = std::chrono::steady_clock;

public:
    using Ptr = std::shared_ptr<Connection>;

    Connection(ConnectionState state, const ConnectionAddr& addr, ConnectionID myID);
    Connection(const Connection& that) = delete;
    Connection(Connection&& that) = delete;
    Connection& operator=(const Connection& that) = delete;
    Connection& operator=(Connection&& that) = delete;
    ~Connection() override;

    [[nodiscard]] bool IsConnected() const override;
    void SetConnected(bool connected) override;
    [[nodiscard]] const IStatistics& GetStatistics() const override;
    [[nodiscard]] Statistics& GetStatistics();

    [[nodiscard]] ConnectionContext& GetContext() override;

    [[nodiscard]] IPacket::List GetFreeSendPackets(std::stop_token stop) override;
    void Send(IPacket::List&& data) override;
    [[nodiscard]] IPacket::List Send2(std::stop_token stop, IPacket::List&& data) override;
    [[nodiscard]] IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) override;
    [[nodiscard]] IPacket::List Recv(std::size_t size, std::stop_token stop, IPacket::List&& freePackets) override;

    void AddFreeRecvPackets(IPacket::List&& freePackets) override;
    void AddFreeSendPackets(IPacket::List&& freePackets) override;

    void Close() override;
    [[nodiscard]] bool IsClosed() const override;
    [[nodiscard]] bool CanBeDeleted() const;

    [[nodiscard]] IPacket::List OnRecvPackets(IPacket::Ptr&& packet);

    [[nodiscard]] const ConnectionKey& GetConnectionKey() const override;
    [[nodiscard]] OutgoingPacket::List GetPacketsToSend();

    void SetInternalFreePackets(IPacket::List&& freeInternalSendPackets, IPacket::List&& freeRecvPackets);

    void ProcessSentPackets(OutgoingPacket::List&& packets);
    void ProcessRecvPackets();

    void Run();

    void SendDataPackets(IPacket::List&& packets) override;
    void SendServicePackets(IPacket::List&& packets) override;
    void ReSendPackets(OutgoingPacket::List&& packets) override;

    [[nodiscard]] IPacket::Ptr RecvPacket(IPacket::Ptr&& packet) override;

    [[nodiscard]] IPacket::List ProcessAcks() override;
    [[nodiscard]] OutgoingPacket::List CheckTimeouts() override;
    void AddAcks(std::span<SeqNumberType> acks) override;
    void SetLastAck(SeqNumberType acks) override;

    IRecvQueue& GetRecvQueue() override;
    IPacket::List GetSendUserData() override;
 
    void SetDestinationID(ConnectionID destinationId) override;
    [[nodiscard]] ConnectionID GetDestinationID() const override;

    [[nodiscard]] IPacket::List GetFreeRecvPackets();

    [[nodiscard]] IPacket::List CleanFreeRecvPackets();
    [[nodiscard]] IPacket::List CleanFreeSendPackets();

    IPacket::Ptr GetFreeSendPacket() override;
    void AddFreeUserSendPackets(IPacket::List&& freePackets) override;

private:
    std::shared_ptr<ConnectionContext> _context;
    ConnectionKey _key;

    IPacket::List _freeInternalSendPackets;
    LockedList<IPacket::Ptr> _freeRecvPackets;
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

    ConnectionState _connectionState;
    std::unordered_map<ConnectionState, std::unique_ptr<IConnectionState>> _states;
    Statistics _statistics;

    LockedList<IPacket::Ptr> _sendUserData;

    ConnectionID _destinationId { 0 };
};
} // namespace FastTransport::Protocol
