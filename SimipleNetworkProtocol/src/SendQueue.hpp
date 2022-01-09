#pragma once

#include "ISendQueue.hpp"

#include <atomic>
#include <vector>

#include "HeaderBuffer.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class IPacket;
class OutgoingPacket;

class SendQueue : public ISendQueue {
    using DataType = std::vector<char>;

public:
    SendQueue()
        : _nextPacketNumber(-1)
    {
    }

    void SendPacket(const DataType& data)
    {
    }

    void SendPacket(IPacket::Ptr&& packet, bool needAck);
    void ReSendPackets(OutgoingPacket::List&& packets);

    // make list of list to get fast 1k packets
    OutgoingPacket::List GetPacketsToSend();

private:
    LockedList<OutgoingPacket> _needToSend;
    std::atomic<SeqNumberType> _nextPacketNumber;

    SpeedController _speedController;
};
} // namespace FastTransport::Protocol
