#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "OutgoingPacket.hpp"
#include "RangedList.hpp"

namespace FastTransport::Protocol {
class Sample {
    using clock = std::chrono::steady_clock;

public:
    explicit Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets);
    IPacket::List ProcessAcks(std::unordered_set<SeqNumberType>& acks);
    OutgoingPacket::List CheckTimeouts();

    bool IsDead() const;
    RangedList GetStats() const;

private:
    std::unordered_map<SeqNumberType, OutgoingPacket> _packets;

    RangedList _stats;
    std::uint16_t _ackPacketNumber;
    std::uint16_t _allPacketsCount;
    std::uint16_t _lostPacketNumber;
    clock::time_point _startTime;
    clock::time_point _endTime;

    SeqNumberType _start;
    SeqNumberType _end;
    std::chrono::microseconds _sendInterval;
};
} // namespace FastTransport::Protocol