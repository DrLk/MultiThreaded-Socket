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

} // namespace FastTransport::Protocol
