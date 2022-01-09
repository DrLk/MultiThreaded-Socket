#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "OutgoingPacket.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class IInflightQueue {
    using clock = std::chrono::steady_clock;

    std::mutex _queueMutex;
    std::unordered_map<SeqNumberType, OutgoingPacket> _queue;
    std::mutex _receivedAcksMutex;
    std::unordered_set<SeqNumberType> _receivedAcks;
    SpeedController _speedController;

public:
    IPacket::List AddQueue(OutgoingPacket::List&& packets);
    IPacket::List ProcessAcks(const SelectiveAckBuffer::Acks& acks);
    OutgoingPacket::List CheckTimeouts();
    size_t GetNumberPacketToSend();
};
} // namespace FastTransport::Protocol
