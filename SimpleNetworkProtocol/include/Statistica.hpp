
#pragma once

#include <atomic>
#include <cstdint>

#include "IStatistica.hpp"

namespace FastTransport::Protocol {
class Statistica final : public IStatistica {
    std::atomic<uint64_t> _lostPackets;
    std::atomic<uint64_t> _sendPackets;
    std::atomic<uint64_t> _receivedPackets;
    std::atomic<uint64_t> _duplicatePackets;

public:
    Statistica();
    virtual ~Statistica();

    virtual uint64_t GetLostPackets() const override;
    virtual uint64_t GetSendPackets() const override;
    virtual uint64_t GetReceivedPackets() const override;
    virtual uint64_t GetDuplicatePackets() const override;
};

} // namespace FastTransport::Protocol
