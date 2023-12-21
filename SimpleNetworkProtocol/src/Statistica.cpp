#include <Statistica.hpp>

#include <cstdint>

namespace FastTransport::Protocol {
Statistica::Statistica() = default;

Statistica::~Statistica() = default;

uint64_t Statistica::GetLostPackets() const
{
    return _lostPackets;
}

uint64_t Statistica::GetSendPackets() const
{
    return _sendPackets;
}

uint64_t Statistica::GetReceivedPackets() const
{
    return _receivedPackets;
}

uint64_t Statistica::GetDuplicatePackets() const
{
    return _duplicatePackets;
}
} // namespace FastTransport::Protocol
