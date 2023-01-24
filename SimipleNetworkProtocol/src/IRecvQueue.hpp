#pragma once

#include <list>
#include <stop_token>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

class IRecvQueue {
public:
    IRecvQueue() = default;
    IRecvQueue(const IRecvQueue&) = default;
    IRecvQueue(IRecvQueue&&) = default;
    IRecvQueue& operator=(const IRecvQueue&) = default;
    IRecvQueue& operator=(IRecvQueue&&) = default;
    virtual ~IRecvQueue() = default;

    [[nodiscard]] virtual IPacket::Ptr AddPacket(IPacket::Ptr&& packet) = 0;
    virtual void ProccessUnorderedPackets() = 0;
    [[nodiscard]] virtual IPacket::List GetUserData(std::stop_token stop) = 0;

    [[nodiscard]] virtual std::list<SeqNumberType> GetSelectiveAcks() = 0;
    [[nodiscard]] virtual SeqNumberType GetLastAck() const = 0;
};

} // namespace FastTransport::Protocol
