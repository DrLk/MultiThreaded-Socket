#include "Sample.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {

Sample::Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets)
    : _packets(std::move(packets))
{
}

IPacket::List Sample::ProcessAcks(std::unordered_set<SeqNumberType>& acks)
{
    IPacket::List freePackets;
    auto now = SampleStats::clock::now();
    for (auto ack = acks.begin(); ack != acks.end();) {
        const auto packet = _packets.find(*ack);
        if (packet != _packets.end()) {

            const SampleStats::clock::duration rtt = now - packet->second._sendTime;
            _timeRangedStats.AddPacket(false, packet->second._sendTime, rtt);

            freePackets.push_back(std::move(packet->second._packet));
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
    for (auto it = _packets.begin(); it != _packets.end();) {
        OutgoingPacket&& packet = std::move(it->second);
        if ((now - packet._sendTime) > timeout) {

            _timeRangedStats.AddPacket(true, packet._sendTime, 0ms);

            needToSend.push_back(std::move(packet));
            it = _packets.erase(it);

        } else {
            it++;
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
        freePackets.push_back(std::move(outgoingPacket._packet));
    }

    _packets.clear();

    return freePackets;
}
} // namespace FastTransport::Protocol