#pragma once

#include <memory>
#include <stop_token>

#include "ConnectionKey.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

using namespace std::chrono_literals;
using FastTransport::Containers::LockedList;

class ISendQueue;
class IInFlightQueue;

class IConnectionState;

class IConnection {
public:
    IConnection() = default;
    IConnection(const IConnection&) = default;
    IConnection(IConnection&&) = default;
    IConnection& operator=(const IConnection&) = default;
    IConnection& operator=(IConnection&&) = default;
    virtual ~IConnection() = default;

    virtual IPacket::List Send(std::stop_token stop, IPacket::List&& data) = 0;
    virtual IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) = 0;
};

class Connection final : public IConnection {
public:
    Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID);
    Connection(const Connection& that) = delete;
    Connection(Connection&& that) = delete;
    Connection& operator=(const Connection& that) = delete;
    Connection& operator=(Connection&& that) = delete;
    ~Connection() override;

    [[nodiscard]] IPacket::List Send(std::stop_token stop, IPacket::List&& data) override;
    [[nodiscard]] IPacket::List Recv(std::stop_token stop, IPacket::List&& freePackets) override;

    [[nodiscard]] IPacket::List OnRecvPackets(IPacket::Ptr&& packet);

    [[nodiscard]] const ConnectionKey& GetConnectionKey() const;
    [[nodiscard]] OutgoingPacket::List GetPacketsToSend();

    void ProcessSentPackets(OutgoingPacket::List&& packets);
    void ProcessRecvPackets();

    void Close();

    void Run();

    void SendPacket(IPacket::Ptr&& packet, bool needAck);
    void ReSendPackets(OutgoingPacket::List&& packets);

    [[nodiscard]] IPacket::List ProcessAcks();
    [[nodiscard]] OutgoingPacket::List CheckTimeouts();
    void AddAcks(const SelectiveAckBuffer::Acks& acks);

    IRecvQueue& GetRecvQueue();
    IPacket::List GetFreeRecvPackets();

    void AddFreeUserSendPackets(IPacket::List&& freePackets);

    IConnectionState* _state; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    IPacket::List _sendUserData; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    std::mutex _sendUserDataMutex; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    ConnectionKey _key; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
    ConnectionID _destinationID { 0 }; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    LockedList<IPacket::Ptr> _freeRecvPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)

    IPacket::List _freeInternalSendPackets; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    static constexpr std::chrono::microseconds DefaultTimeOut = 100ms;

    LockedList<std::vector<char>> _recvUserData;

    std::chrono::microseconds _lastReceivedPacket;

    std::unique_ptr<IInFlightQueue> _inFlightQueue;
    std::unique_ptr<IRecvQueue> _recvQueue;
    std::unique_ptr<ISendQueue> _sendQueue;

    LockedList<IPacket::Ptr> _freeUserSendPackets;
};
} // namespace FastTransport::Protocol
