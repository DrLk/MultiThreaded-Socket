#include <Statistica.hpp>

#include <cstdint>

namespace FastTransport::Protocol {
Statistica::Statistica() = default;

Statistica::~Statistica() = default;

void Statistica::AddLostPackets(uint64_t lostPackets)
{
    _lostPackets += lostPackets;
}

uint64_t Statistica::GetLostPackets() const
{
    return _lostPackets;
}

void Statistica::AddSendPackets(uint64_t sendPackets /*= OnePacket*/)
{
    _sendPackets += sendPackets;
}

uint64_t Statistica::GetSendPackets() const
{
    return _sendPackets;
}

void Statistica::AddReceivedPackets(uint64_t receivedPackets)
{
    _receivedPackets += receivedPackets;
}

uint64_t Statistica::GetReceivedPackets() const
{
    return _receivedPackets;
}

void Statistica::AddDuplicatePackets(uint64_t duplicatePackets /* = OnePacket*/)
{
    _duplicatePackets += duplicatePackets;
}

uint64_t Statistica::GetDuplicatePackets() const
{
    return _duplicatePackets;
}

void Statistica::AddFullPackets(uint64_t duplicatePackets /* = OnePacket*/)
{
    _duplicatePackets += duplicatePackets;
}

uint64_t Statistica::GetFullPackets() const
{
    return _duplicatePackets;
}

void Statistica::AddAckSendPackets(uint64_t ackSendPackets /*= OnePacket*/)
{
    _ackSendPackets += ackSendPackets;
}

uint64_t Statistica::GetAckSendPackets() const
{
    return _ackSendPackets;
}

void Statistica::AddAckReceivedPackets(uint64_t ackReceivedPackets /*= OnePacket*/)
{
    _ackReceivedPackets += ackReceivedPackets;
}

uint64_t Statistica::GetAckReceivedPackets() const
{
    return _ackReceivedPackets;
}
} // namespace FastTransport::Protocol
