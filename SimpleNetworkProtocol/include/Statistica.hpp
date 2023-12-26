
#pragma once

#include <atomic>
#include <cstdint>

#include "IStatistica.hpp"

namespace FastTransport::Protocol {
class Statistica final : public IStatistica {
public:
    Statistica();
    ~Statistica() override;

    void AddLostPackets(uint64_t lostPackets = OnePacket);
    uint64_t GetLostPackets() const override;
    void AddSendPackets(uint64_t sendPackets = OnePacket);
    uint64_t GetSendPackets() const override;
    void AddReceivedPackets(uint64_t receivedPackets = OnePacket);
    uint64_t GetReceivedPackets() const override;
    void AddDuplicatePackets(uint64_t duplicatePackets = OnePacket);
    uint64_t GetDuplicatePackets() const override;
    void AddOverflowPackets(uint64_t overflowPackets = OnePacket);
    uint64_t GetOverflowPackets() const override;
    void AddAckSendPackets(uint64_t ackSendPacket = OnePacket);
    uint64_t GetAckSendPackets() const override;
    void AddAckReceivedPackets(uint64_t ackReceivedPackets = OnePacket);
    uint64_t GetAckReceivedPackets() const override;

private:
    static constexpr uint64_t OnePacket = 1;

    std::atomic<uint64_t> _lostPackets;
    std::atomic<uint64_t> _sendPackets;
    std::atomic<uint64_t> _receivedPackets;
    std::atomic<uint64_t> _duplicatePackets;
    std::atomic<uint64_t> _overflowPackets;
    std::atomic<uint64_t> _ackSendPackets;
    std::atomic<uint64_t> _ackReceivedPackets;
};

} // namespace FastTransport::Protocol
