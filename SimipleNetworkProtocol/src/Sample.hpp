#pragma once

#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include "OutgoingPacket.hpp"
#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {
class Sample {
    using clock = std::chrono::steady_clock;

public:
    explicit Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets);
    IPacket::List ProcessAcks(std::unordered_set<SeqNumberType>& acks);
    OutgoingPacket::List CheckTimeouts(clock::duration timeout);

    bool IsDead() const;
    TimeRangedStats GetStats() const;

    IPacket::List FreePackets();

private:
    std::unordered_map<SeqNumberType, OutgoingPacket> _packets;

    TimeRangedStats _timeRangedStats;
};
} // namespace FastTransport::Protocol