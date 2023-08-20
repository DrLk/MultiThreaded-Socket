#include "SendQueue.hpp"

#include <limits>
#include <utility>

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

SendQueue::SendQueue()
    : _nextPacketNumber(-1)
{
}

void SendQueue::SendPacket(IPacket::Ptr&& packet, bool needAck)
{
    packet->SetMagic();

    if (needAck) {
        packet->SetSeqNumber(++_nextPacketNumber);
    } else {
        packet->SetSeqNumber((std::numeric_limits<SeqNumberType>::max)());
    }

    {
        _needToSend.push_back(OutgoingPacket(std::move(packet), needAck));
    }
}

void SendQueue::ReSendPackets(OutgoingPacket::List&& packets)
{
    _resendPackets.splice(std::move(packets));
}

OutgoingPacket::List SendQueue::GetPacketsToSend(size_t size)
{
    OutgoingPacket::List result;

    if (_resendPackets.size() > size) {
        result.splice(_resendPackets.TryGenerate(size));
        return result;
    }

    result.swap(_resendPackets);
    size -= result.size();

    if (size != 0U) {
        result.splice(_needToSend.TryGenerate(size));
    }

    return result;
}

bool SendQueue::OutgoingComparator(const OutgoingPacket& left, const OutgoingPacket& right)
{
    return left.GetPacket()->GetSeqNumber() < right.GetPacket()->GetSeqNumber();
};
} // namespace FastTransport::Protocol
