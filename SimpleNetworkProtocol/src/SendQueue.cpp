#include "SendQueue.hpp"

#include <cstddef>
#include <limits>
#include <utility>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

SendQueue::SendQueue()
    : _resendPackets(OutgoingComparator)
    , _nextPacketNumber(-1)
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

void SendQueue::ReSendPackets(OutgoingPacket::List&& packets) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    for (auto& packet : packets) {
        _resendPackets.insert(std::move(packet));
    }
}

OutgoingPacket::List SendQueue::GetPacketsToSend(size_t size)
{
    OutgoingPacket::List result;

    for (auto it = _resendPackets.begin(); it != _resendPackets.end() && size > 0; size--) {
        result.push_back(std::move(_resendPackets.extract(it++).value()));
    }

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
