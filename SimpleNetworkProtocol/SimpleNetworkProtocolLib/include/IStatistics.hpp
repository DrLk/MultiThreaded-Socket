#pragma once

#include <cstdint>
#include <ostream>

namespace FastTransport::Protocol {

class IStatistics {
public:
    IStatistics();
    IStatistics(const IStatistics&) = default;
    IStatistics(IStatistics&&) = delete;
    IStatistics& operator=(const IStatistics&) = default;
    IStatistics& operator=(IStatistics&&) = delete;
    virtual ~IStatistics();
    [[nodiscard]] virtual uint64_t GetLostPackets() const = 0;
    [[nodiscard]] virtual uint64_t GetSendPackets() const = 0;
    [[nodiscard]] virtual uint64_t GetReceivedPackets() const = 0;
    [[nodiscard]] virtual uint64_t GetDuplicatePackets() const = 0;
    [[nodiscard]] virtual uint64_t GetOverflowPackets() const = 0;
    [[nodiscard]] virtual uint64_t GetAckSendPackets() const = 0;
    [[nodiscard]] virtual uint64_t GetAckReceivedPackets() const = 0;
};

std::ostream& operator<<(std::ostream& stream, const IStatistics& statistics); // NOLINT(fuchsia-overloaded-operator)

} // namespace FastTransport::Protocol
