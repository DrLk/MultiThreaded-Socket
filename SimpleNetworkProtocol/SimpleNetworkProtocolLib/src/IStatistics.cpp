#include "IStatistics.hpp"

#include <ostream>

namespace FastTransport::Protocol {

IStatistics::IStatistics() = default;

IStatistics::~IStatistics() = default;

std::ostream& operator<<(std::ostream& stream, const IStatistics& statistics) // NOLINT(fuchsia-overloaded-operator)
{
    stream << "Send: " << statistics.GetSendPackets()
           << " Received: " << statistics.GetReceivedPackets()
           << " AckSend: " << statistics.GetAckSendPackets()
           << " AckReceived: " << statistics.GetAckReceivedPackets()
           << " Lost: " << statistics.GetLostPackets()
           << " Overflow: " << statistics.GetOverflowPackets()
           << " Duplicates: " << statistics.GetDuplicatePackets();
    return stream;
}

} // namespace FastTransport::Protocol
