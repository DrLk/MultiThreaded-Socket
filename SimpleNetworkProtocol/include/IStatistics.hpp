#pragma once

#include <cstdint>
#include <ostream>

namespace FastTransport::Protocol {

class IStatistics {
public:
    virtual ~IStatistics() = default;
    virtual uint64_t GetLostPackets() const = 0;
    virtual uint64_t GetSendPackets() const = 0;
    virtual uint64_t GetReceivedPackets() const = 0;
    virtual uint64_t GetDuplicatePackets() const = 0;
    virtual uint64_t GetOverflowPackets() const = 0;
    virtual uint64_t GetAckSendPackets() const = 0;
    virtual uint64_t GetAckReceivedPackets() const = 0;
};

std::ostream& operator<<(std::ostream& stream, const IStatistics& statistics); // NOLINT(fuchsia-overloaded-operator)

} // FastTransport::Protocol
