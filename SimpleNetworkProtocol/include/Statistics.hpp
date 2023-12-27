
#pragma once

#include <atomic>
#include <cstdint>

#include "IStatistics.hpp"

namespace FastTransport::Protocol {
class Statistics final : public IStatistics {
public:
    Statistics();
    Statistics(const Statistics&) = delete;
    Statistics(Statistics&&) = delete;
    Statistics& operator=(const Statistics&) = delete;
    Statistics& operator=(Statistics&&) = delete;
    ~Statistics() override;

    void AddLostPackets(uint64_t lostPackets);
    [[nodiscard]] uint64_t GetLostPackets() const override;
    void AddSendPackets();
    [[nodiscard]] uint64_t GetSendPackets() const override;
    void AddReceivedPackets();
    [[nodiscard]] uint64_t GetReceivedPackets() const override;
    void AddDuplicatePackets();
    [[nodiscard]] uint64_t GetDuplicatePackets() const override;
    void AddOverflowPackets();
    [[nodiscard]] uint64_t GetOverflowPackets() const override;
    void AddAckSendPackets();
    [[nodiscard]] uint64_t GetAckSendPackets() const override;
    void AddAckReceivedPackets();
    [[nodiscard]] uint64_t GetAckReceivedPackets() const override;

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
