#pragma once

#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {
class IInFlightQueue {
public:
    [[nodiscard]] virtual IPacket::List AddQueue(OutgoingPacket::List&& packets) = 0;
    virtual void AddAcks(const SelectiveAckBuffer::Acks& acks) = 0;
    [[nodiscard]] virtual IPacket::List ProcessAcks() = 0;
    [[nodiscard]] virtual OutgoingPacket::List CheckTimeouts() = 0;
    [[nodiscard]] virtual size_t GetNumberPacketToSend() = 0;
};
} // namespace FastTransport::Protocol
