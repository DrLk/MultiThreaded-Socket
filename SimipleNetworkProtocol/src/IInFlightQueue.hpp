#pragma once

#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {
class IInFlightQueue {
public:
    IInFlightQueue() = default;
    IInFlightQueue(const IInFlightQueue&) = default;
    IInFlightQueue(IInFlightQueue&&) = default;
    IInFlightQueue& operator=(const IInFlightQueue&) = default;
    IInFlightQueue& operator=(IInFlightQueue&&) = default;
    virtual ~IInFlightQueue() = default;

    [[nodiscard]] virtual std::pair<IPacket::List, IPacket::List> AddQueue(OutgoingPacket::List&& packets) = 0;
    virtual void AddAcks(const SelectiveAckBuffer::Acks& acks) = 0;
    [[nodiscard]] virtual IPacket::List ProcessAcks() = 0;
    [[nodiscard]] virtual OutgoingPacket::List CheckTimeouts() = 0;
    [[nodiscard]] virtual size_t GetNumberPacketToSend() = 0;
    [[nodiscard]] virtual IPacket::List GetAllPackets() = 0;
};
} // namespace FastTransport::Protocol
