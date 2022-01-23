#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <unordered_set>

#include "OutgoingPacket.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class IInflightQueue {
    using clock = std::chrono::steady_clock;

    std::mutex _receivedAcksMutex;
    std::unordered_set<SeqNumberType> _receivedAcks;
    SpeedController _speedController;
    std::list<Sample> _samples;

public:
    IPacket::List AddQueue(OutgoingPacket::List&& packets);
    void AddAcks(const SelectiveAckBuffer::Acks& acks);
    IPacket::List ProcessAcks();
    OutgoingPacket::List CheckTimeouts();
    size_t GetNumberPacketToSend();
};
} // namespace FastTransport::Protocol
