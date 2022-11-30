#include "SendQueue.hpp"

#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

#include <limits>

namespace FastTransport::Protocol {

SendQueue::SendQueue()
    : _nextPacketNumber(-1)
    , _resendPackets(OutgoingComparator)
{
}

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
        _needToSend.push_back(OutgoingPacket(std::move(packet), needAck));
    }
}

void SendQueue::ReSendPackets(OutgoingPacket::List&& packets)
{
    for (auto&& packet : packets) {
        _resendPackets.insert(std::move(packet));
    }
}

OutgoingPacket::List SendQueue::GetPacketsToSend(size_t size)
{
    OutgoingPacket::List result;

    {
        for (auto it = _resendPackets.begin(); it != _resendPackets.end() && size > 0; size--) {
            result.push_back(std::move(_resendPackets.extract(it++).value()));
        }
    }

    if (size != 0U) {
        result.splice(std::move(_needToSend.TryGenerate(size)));
    }

    return result;
}

bool SendQueue::OutgoingComparator(const OutgoingPacket& left, const OutgoingPacket& right)
{
    return left._packet->GetHeader().GetSeqNumber() < right._packet->GetHeader().GetSeqNumber();
};
} // namespace FastTransport::Protocol
