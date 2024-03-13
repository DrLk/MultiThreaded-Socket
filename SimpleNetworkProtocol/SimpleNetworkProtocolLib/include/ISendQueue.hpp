#pragma once

#include <cstddef>

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

class ISendQueue {
public:
    ISendQueue() = default;
    ISendQueue(const ISendQueue&) = default;
    ISendQueue(ISendQueue&&) = default;
    ISendQueue& operator=(const ISendQueue&) = default;
    ISendQueue& operator=(ISendQueue&&) = default;
    virtual ~ISendQueue() = default;

    virtual void SendPackets(IPacket::List&& packets, bool needAck) = 0;
    virtual void ReSendPackets(OutgoingPacket::List&& packets) = 0;
    [[nodiscard]] virtual OutgoingPacket::List GetPacketsToSend(size_t size) = 0;
};
} // namespace FastTransport::Protocol
