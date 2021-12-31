#include "SendQueue.hpp"

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

#include <limits>

namespace FastTransport::Protocol {
void SendQueue::SendPacket(IPacket::Ptr&& packet, bool needAck)
{
    Header header = packet->GetHeader();
    header.SetMagic();

    if (needAck) {
        header.SetSeqNumber(++_nextPacketNumber);
    } else {
        header.SetSeqNumber((std::numeric_limits<SeqNumberType>::max)());
    }

    {
        std::lock_guard lock(_needToSend._mutex);
        _needToSend.push_back(OutgoingPacket(std::move(packet), needAck));
    }
}

void SendQueue::ReSendPackets(OutgoingPacket::List&& packets)
{
    std::lock_guard lock(_needToSend._mutex);
    _needToSend.splice(std::move(packets));
}

// make list of list to get fast 1k packets
OutgoingPacket::List SendQueue::GetPacketsToSend()
{
    OutgoingPacket::List result;
    {
        std::lock_guard lock(_needToSend._mutex);
        result = std::move(_needToSend);
    }

    return result;
}
} // namespace FastTransport::Protocol
