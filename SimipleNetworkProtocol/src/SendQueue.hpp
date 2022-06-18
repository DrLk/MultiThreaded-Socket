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
    OutgoingPacket::List GetPacketsToSend(size_t size);

private:
    static constexpr auto outgoingComparator = [](const OutgoingPacket& left, const OutgoingPacket& right) {
        return left._packet->GetHeader().GetSeqNumber() < right._packet->GetHeader().GetSeqNumber();
    };
    std::set<OutgoingPacket, decltype(outgoingComparator)> _resendPackets;

    Containers::MultiList<OutgoingPacket> _needToSend;
    std::atomic<SeqNumberType> _nextPacketNumber;
};
} // namespace FastTransport::Protocol
