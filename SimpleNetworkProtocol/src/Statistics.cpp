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

void Statistics::AddSendPackets(uint64_t sendPackets)
{
    _sendPackets += sendPackets;
}

uint64_t Statistics::GetSendPackets() const
{
    return _sendPackets;
}

void Statistics::AddReceivedPackets()
{
    _receivedPackets += OnePacket;
}

uint64_t Statistics::GetReceivedPackets() const
{
    return _receivedPackets;
}

void Statistics::AddDuplicatePackets()
{
    _duplicatePackets += OnePacket;
}

uint64_t Statistics::GetDuplicatePackets() const
{
    return _duplicatePackets;
}

void Statistics::AddOverflowPackets()
{
    _overflowPackets += OnePacket;
}

uint64_t Statistics::GetOverflowPackets() const
{
    return _overflowPackets;
}

void Statistics::AddAckSendPackets()
{
    _ackSendPackets += OnePacket;
}

uint64_t Statistics::GetAckSendPackets() const
{
    return _ackSendPackets;
}

void Statistics::AddAckReceivedPackets()
{
    _ackReceivedPackets += OnePacket;
}

uint64_t Statistics::GetAckReceivedPackets() const
{
    return _ackReceivedPackets;
}
} // namespace FastTransport::Protocol
