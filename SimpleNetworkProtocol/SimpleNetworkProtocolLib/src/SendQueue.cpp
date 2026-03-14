#include "SendQueue.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport::Protocol {

SendQueue::SendQueue()
    : _nextPacketNumber(0)
{
}

void SendQueue::SendPackets(IPacket::List&& packets, bool needAck) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    for (auto&& packet : packets) {
        packet->SetMagic();

        if (needAck) {
            packet->SetSeqNumber(++_nextPacketNumber);
            _needToSend.push_back(OutgoingPacket(std::move(packet), needAck));
        } else {
            packet->SetSeqNumber(~SeqNumberType {});
            _serviceNeedToSend.push_back(OutgoingPacket(std::move(packet), needAck));
        }
    }
}

void SendQueue::ReSendPackets(OutgoingPacket::List&& packets) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    for (auto& packet : packets) {
        _resendPackets.push_back(std::move(packet));
        std::ranges::push_heap(_resendPackets, OutgoingComparator {});
    }
}

OutgoingPacket::List SendQueue::GetPacketsToSend(size_t size)
{
    OutgoingPacket::List result;

    while (!_resendPackets.empty() && size > 0) {
        std::ranges::pop_heap(_resendPackets, OutgoingComparator {});
        result.push_back(std::move(_resendPackets.back()));
        _resendPackets.pop_back();
        size--;
    }

    if (size != 0U) {
        result.splice(_needToSend.TryGenerate(size));
    }

    return result;
}

OutgoingPacket::List SendQueue::GetServicePacketsToSend()
{
    OutgoingPacket::List result;
    result.splice(_serviceNeedToSend.TryGenerate(std::numeric_limits<size_t>::max()));
    return result;
}

bool SendQueue::OutgoingComparator::operator()(const OutgoingPacket& left, const OutgoingPacket& right) const // NOLINT(fuchsia-overloaded-operator)
{
    // Reversed for min-heap: smallest seq number at the top
    return left.GetPacket()->GetSeqNumber() > right.GetPacket()->GetSeqNumber();
}
} // namespace FastTransport::Protocol
