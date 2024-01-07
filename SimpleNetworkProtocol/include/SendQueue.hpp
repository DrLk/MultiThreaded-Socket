#pragma once

#include <atomic>
#include <cstddef>
#include <set>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "ISendQueue.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::MultiList;

class SendQueue final : public ISendQueue {
public:
    SendQueue();

    void SendPacket(IPacket::Ptr&& packet, bool needAck) override;
    void ReSendPackets(OutgoingPacket::List&& packets) override;

    // make list of list to get fast 1k packets
    [[nodiscard]] OutgoingPacket::List GetPacketsToSend(size_t size) override;

private:
    static bool OutgoingComparator(const OutgoingPacket& left, const OutgoingPacket& right);
    std::set<OutgoingPacket, decltype(&OutgoingComparator)> _resendPackets;

    MultiList<OutgoingPacket> _needToSend;
    std::atomic<SeqNumberType> _nextPacketNumber;
};
} // namespace FastTransport::Protocol
