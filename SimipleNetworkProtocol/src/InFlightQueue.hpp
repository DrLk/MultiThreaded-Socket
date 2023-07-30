#pragma once

#include "IInFlightQueue.hpp"

#include <chrono>
#include <mutex>
#include <span>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "Sample.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class InFlightQueue final : public IInFlightQueue {
public:
    [[nodiscard]] std::pair<IPacket::List, IPacket::List> AddQueue(OutgoingPacket::List&& packets) override;
    void AddAcks(std::span<SeqNumberType> acks) override;
    [[nodiscard]] IPacket::List ProcessAcks() override;
    [[nodiscard]] OutgoingPacket::List CheckTimeouts() override;
    [[nodiscard]] size_t GetNumberPacketToSend() override;
    [[nodiscard]] IPacket::List GetAllPackets() override;

private:
    using clock = std::chrono::steady_clock;

    std::mutex _receivedAcksMutex;
    std::unordered_set<SeqNumberType> _receivedAcks;
    SpeedController _speedController;
    std::list<Sample> _samples;
};
} // namespace FastTransport::Protocol
