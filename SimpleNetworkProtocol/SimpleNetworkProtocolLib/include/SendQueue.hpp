#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "ISendQueue.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::MultiList;

class SendQueue final : public ISendQueue {
public:
    SendQueue();

    void SendPackets(IPacket::List&& packets, bool needAck) override;
    void ReSendPackets(OutgoingPacket::List&& packets) override;

    // make list of list to get fast 1k packets
    [[nodiscard]] OutgoingPacket::List GetPacketsToSend(size_t size) override;
    [[nodiscard]] OutgoingPacket::List GetServicePacketsToSend() override;

private:
    struct OutgoingComparator {
        bool operator()(const OutgoingPacket& left, const OutgoingPacket& right) const; // NOLINT(fuchsia-overloaded-operator)
    };
    std::vector<OutgoingPacket> _resendPackets;

    MultiList<OutgoingPacket> _needToSend;
    MultiList<OutgoingPacket> _serviceNeedToSend;
    std::atomic<SeqNumberType> _nextPacketNumber;
};
} // namespace FastTransport::Protocol
