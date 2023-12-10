#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <list>
#include <span>
#include <unordered_set>
#include <utility>

#include "HeaderTypes.hpp"
#include "IInFlightQueue.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "Sample.hpp"
#include "SpeedController.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Protocol {
class InFlightQueue final : public IInFlightQueue {
public:
    [[nodiscard]] std::pair<IPacket::List, IPacket::List> AddQueue(OutgoingPacket::List&& packets) override;
    void SetLastAck(SeqNumberType lastAck) override;
    void AddAcks(std::span<SeqNumberType> acks) override;
    [[nodiscard]] IPacket::List ProcessAcks() override;
    [[nodiscard]] OutgoingPacket::List CheckTimeouts() override;
    [[nodiscard]] size_t GetNumberPacketToSend() override;
    [[nodiscard]] IPacket::List GetAllPackets() override;

private:
    using clock = std::chrono::steady_clock;

    Thread::SpinLock _receivedAcksMutex;
    std::unordered_set<SeqNumberType> _receivedAcks;
    SpeedController _speedController;
    std::list<Sample> _samples;

    std::atomic<SeqNumberType> _lastAckNumber { 0 };
};
} // namespace FastTransport::Protocol
