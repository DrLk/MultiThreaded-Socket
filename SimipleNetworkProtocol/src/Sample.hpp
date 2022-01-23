#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {
class Sample {
    using clock = std::chrono::steady_clock;

public:
    explicit Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets);
    IPacket::List ProcessAcks(std::unordered_set<SeqNumberType>& acks);
    OutgoingPacket::List CheckTimeouts();

    bool IsDead() const;

private:
    std::unordered_map<SeqNumberType, OutgoingPacket> _packets;
    std::uint16_t _ackPacketNumber;
    std::uint16_t _lostPacketNumber;

    SeqNumberType _start;
    SeqNumberType _end;
    std::chrono::microseconds _sendInterval;
    std::chrono::microseconds _startTime;
};
} // namespace FastTransport::Protocol