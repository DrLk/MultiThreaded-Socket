#pragma once

#include "HeaderTypes.hpp"
#include "IConnection.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

class IRecvQueue;
class ConnectionKey;

class IConnectionInternal : public IConnection {
public:
    virtual void SetConnected(bool connected) = 0;

    virtual void SendDataPackets(IPacket::List&& packets) = 0;
    virtual void SendServicePackets(IPacket::List&& packets) = 0;
    virtual void ReSendPackets(OutgoingPacket::List&& packets) = 0;

    [[nodiscard]] virtual IPacket::Ptr RecvPacket(IPacket::Ptr&& packet) = 0;

    [[nodiscard]] virtual IPacket::List ProcessAcks() = 0;
    [[nodiscard]] virtual OutgoingPacket::List CheckTimeouts() = 0;
    virtual void AddAcks(std::span<SeqNumberType> acks) = 0;
    virtual void SetLastAck(SeqNumberType acks) = 0;

    virtual IRecvQueue& GetRecvQueue() = 0;
    virtual IPacket::List GetSendUserData() = 0;

    virtual void SetDestinationID(ConnectionID destinationId) = 0;
    [[nodiscard]] virtual ConnectionID GetDestinationID() const = 0;

    [[nodiscard]] virtual const ConnectionKey& GetConnectionKey() const = 0;

    virtual IPacket::Ptr GetFreeSendPacket() = 0;
    virtual void AddFreeUserSendPackets(IPacket::List&& freePackets) = 0;
    virtual void NotifySendPacketsEvent() const = 0;
};
} // namespace FastTransport::Protocol
