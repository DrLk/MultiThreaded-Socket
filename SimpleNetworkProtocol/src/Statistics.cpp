#include <Statistics.hpp>

#include <cstdint>

namespace FastTransport::Protocol {
Statistics::Statistics() = default;

Statistics::~Statistics() = default;

void Statistics::AddLostPackets(uint64_t lostPackets)
{
    _lostPackets += lostPackets;
}

uint64_t Statistics::GetLostPackets() const
{
    return _lostPackets;
}

void Statistics::AddSendPackets(uint64_t sendPackets /*= OnePacket*/)
{
    _sendPackets += sendPackets;
}

uint64_t Statistics::GetSendPackets() const
{
    return _sendPackets;
}

void Statistics::AddReceivedPackets(uint64_t receivedPackets)
{
    _receivedPackets += receivedPackets;
}

uint64_t Statistics::GetReceivedPackets() const
{
    return _receivedPackets;
}

void Statistics::AddDuplicatePackets(uint64_t duplicatePackets /* = OnePacket*/)
{
    _duplicatePackets += duplicatePackets;
}

uint64_t Statistics::GetDuplicatePackets() const
{
    return _duplicatePackets;
}

void Statistics::AddOverflowPackets(uint64_t overflowPackets /* = OnePacket*/)
{
    _overflowPackets += overflowPackets;
}

uint64_t Statistics::GetOverflowPackets() const
{
    return _overflowPackets;
}

void Statistics::AddAckSendPackets(uint64_t ackSendPackets /*= OnePacket*/)
{
    _ackSendPackets += ackSendPackets;
}

uint64_t Statistics::GetAckSendPackets() const
{
    return _ackSendPackets;
}

void Statistics::AddAckReceivedPackets(uint64_t ackReceivedPackets /*= OnePacket*/)
{
    _ackReceivedPackets += ackReceivedPackets;
}

uint64_t Statistics::GetAckReceivedPackets() const
{
    return _ackReceivedPackets;
}
} // namespace FastTransport::Protocol
