#pragma once

#include "ISendQueue.hpp"

#include <atomic>
#include <set>
#include <vector>

#include "HeaderBuffer.hpp"
#include "MultiList.hpp"
#include "OutgoingPacket.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
class SendQueue : public ISendQueue {
    using DataType = std::vector<char>;

public:
    SendQueue();

    void SendPacket(IPacket::Ptr&& packet, bool needAck);
    void ReSendPackets(OutgoingPacket::List&& packets);

    // make list of list to get fast 1k packets
    OutgoingPacket::List GetPacketsToSend(size_t size);

private:
    static bool OutgoingComparator(const OutgoingPacket& left, const OutgoingPacket& right);
    std::set<OutgoingPacket, decltype(&OutgoingComparator)> _resendPackets;

    Containers::MultiList<OutgoingPacket> _needToSend;
    std::atomic<SeqNumberType> _nextPacketNumber;
};
} // namespace FastTransport::Protocol
