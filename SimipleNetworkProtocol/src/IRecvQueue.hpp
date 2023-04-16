#pragma once

#include <list>

#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::LockedList;

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
    virtual LockedList<IPacket::Ptr>& GetUserData() = 0;

    [[nodiscard]] virtual std::list<SeqNumberType> GetSelectiveAcks() = 0;
    [[nodiscard]] virtual SeqNumberType GetLastAck() const = 0;
};

} // namespace FastTransport::Protocol
