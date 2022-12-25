#pragma once

#include "IInFlightQueue.hpp"

#include <chrono>

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "Sample.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class InflightQueue final : public IInFlightQueue {
    using clock = std::chrono::steady_clock;

    std::mutex _receivedAcksMutex;
    std::unordered_set<SeqNumberType> _receivedAcks;
    SpeedController _speedController;
    std::list<Sample> _samples;

public:
    [[nodiscard]] IPacket::List AddQueue(OutgoingPacket::List&& packets) override;
    void AddAcks(const SelectiveAckBuffer::Acks& acks) override;
    [[nodiscard]] IPacket::List ProcessAcks() override;
    [[nodiscard]] OutgoingPacket::List CheckTimeouts() override;
    [[nodiscard]] size_t GetNumberPacketToSend() override;
};
} // namespace FastTransport::Protocol
