#pragma once

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

class ISendQueue {
public:
    virtual void SendPacket(IPacket::Ptr&& packet, bool needAck) = 0;
    virtual void ReSendPackets(OutgoingPacket::List&& packets) = 0;
    [[nodiscard]] virtual OutgoingPacket::List GetPacketsToSend(size_t size) = 0;
};
} // namespace FastTransport::Protocol
