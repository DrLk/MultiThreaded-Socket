#include "Sample.hpp"

#include <chrono>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "SampleStats.hpp"
#include "TimeRangedStats.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {

Sample::Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets, size_t servicePacketNumber, size_t doubleSentPacketNumber)
    : _packets(std::move(packets))
    , _servicePacketNumber(servicePacketNumber)
    , _doubleSentPacketNumber(doubleSentPacketNumber)
{
}

IPacket::List Sample::ProcessAcks(std::unordered_set<SeqNumberType>& acks, SeqNumberType lastAckNumber)
{
    auto now = SampleStats::clock::now();
    IPacket::List freePackets;
    for (auto packet = _packets.begin(); packet != _packets.end();) {
        if (packet->first <= lastAckNumber) {

            const SampleStats::clock::duration rtt = now - packet->second.GetSendTime();
            _timeRangedStats.AddPacket(false, packet->second.GetSendTime(), rtt);

            freePackets.push_back(std::move(packet->second.GetPacket()));
            packet = _packets.erase(packet);
        } else {
            ++packet;
        }
    }

    for (auto ack = acks.begin(); ack != acks.end();) {
        if (*ack <= lastAckNumber) {
            ack = acks.erase(ack);
            continue;
        }

        const auto packet = _packets.find(*ack);
        if (packet != _packets.end()) {

            const SampleStats::clock::duration rtt = now - packet->second.GetSendTime();
            _timeRangedStats.AddPacket(false, packet->second.GetSendTime(), rtt);

            freePackets.push_back(std::move(packet->second.GetPacket()));
            _packets.erase(packet);
            ack = acks.erase(ack);

        } else {
            ack++;
        }
    }

    return freePackets;
}

OutgoingPacket::List Sample::CheckTimeouts(clock::duration timeout)
{
    OutgoingPacket::List needToSend;

    if (_packets.empty()) {
        return needToSend;
    }

    const clock::time_point now = clock::now();
    for (auto iterator = _packets.begin(); iterator != _packets.end();) {
        OutgoingPacket&& packet = std::move(iterator->second);
        if ((now - packet.GetSendTime()) > timeout) {

            _timeRangedStats.AddPacket(true, packet.GetSendTime(), 0ms);

            needToSend.push_back(std::move(packet));
            iterator = _packets.erase(iterator);

        } else {
            iterator++;
        }
    }

    return needToSend;
}

bool Sample::IsDead() const
{
    const bool isEmpty = _packets.empty();
    return isEmpty;
}

TimeRangedStats Sample::GetStats() const
{
    return _timeRangedStats;
}

IPacket::List Sample::FreePackets()
{
    IPacket::List freePackets;

    for (auto& [seqNumber, outgoingPacket] : _packets) {
        freePackets.push_back(std::move(outgoingPacket.GetPacket()));
    }

    _packets.clear();

    return freePackets;
}
} // namespace FastTransport::Protocol