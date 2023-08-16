#include "pch.hpp"
#include "Packet.hpp"

#include "HeaderBuffer.hpp"

namespace FastTransport::Protocol {

Packet::Packet(int size)
    : _element(size)
    , _time(0)
{
}

std::span<SeqNumberType> Packet::GetAcks()
{
    return SelectiveAckBuffer::Acks(_element.data(), _element.size()).GetAcks();
}

void Packet::SetAcks(const std::list<SeqNumberType>& acks)
{
    SelectiveAckBuffer::Acks(_element.data(), _element.size()).SetAcks(acks);
}

PacketType Packet::GetPacketType() const
{
    return Header(_element.data(), _element.size()).GetPacketType();
}

void Packet::SetPacketType(PacketType type)
{
    Header(_element.data(), _element.size()).SetPacketType(type);
}

[[nodiscard]] ConnectionID Packet::GetSrcConnectionID() const
{
    return Header(_element.data(), _element.size()).GetSrcConnectionID();
}

void Packet::SetSrcConnectionID(ConnectionID connectionId)
{
    Header(_element.data(), _element.size()).SetSrcConnectionID(connectionId);
}

[[nodiscard]] ConnectionID Packet::GetDstConnectionID() const
{
    return Header(_element.data(), _element.size()).GetDstConnectionID();
}

void Packet::SetDstConnectionID(ConnectionID connectionId)
{
    Header(_element.data(), _element.size()).SetDstConnectionID(connectionId);
}

[[nodiscard]] SeqNumberType Packet::GetSeqNumber() const
{
    return Header(_element.data(), _element.size()).GetSeqNumber();
}

void Packet::SetSeqNumber(SeqNumberType seq)
{
    Header(_element.data(), _element.size()).SetSeqNumber(seq);
}

[[nodiscard]] SeqNumberType Packet::GetAckNumber() const
{
    return Header(_element.data(), _element.size()).GetAckNumber();
}

void Packet::SetAckNumber(SeqNumberType ack)
{
    Header(_element.data(), _element.size()).SetAckNumber(ack);
}

void Packet::SetMagic()
{
    return Header(_element.data(), _element.size()).SetMagic();
}

bool Packet::IsValid() const
{
    return Header(_element.data(), _element.size()).IsValid();
}

} // namespace FastTransport::Protocol
