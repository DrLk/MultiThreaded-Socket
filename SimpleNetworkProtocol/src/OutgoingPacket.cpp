#include "OutgoingPacket.hpp"

#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

OutgoingPacket::OutgoingPacket(IPacket::Ptr&& packet, bool needAck)
    : _packet(std::move(packet))
    , _needAck(needAck)
{
}

const IPacket::Ptr& OutgoingPacket::GetPacket() const
{
    return _packet;
}

IPacket::Ptr& OutgoingPacket::GetPacket()
{
    return _packet;
}

bool OutgoingPacket::NeedAck() const
{
    return _needAck;
}

OutgoingPacket::clock::time_point OutgoingPacket::GetSendTime() const
{
    return _sendTime;
}

void OutgoingPacket::SetSendTime(clock::time_point sendTime)
{
    _sendTime = sendTime;
}

} // namespace FastTransport::Protocol
