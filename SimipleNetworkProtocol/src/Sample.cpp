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
    for (auto ack = acks.begin(); ack != acks.end();) {
        const auto packet = _packets.find(*ack);
        if (packet != _packets.end()) {

            _timeRangedStats.AddPacket(false, packet->second._sendTime);

            freePackets.push_back(std::move(packet->second._packet));
            _packets.erase(packet);
            ack = acks.erase(ack);

        } else {
            ack++;
        }
    }

    return freePackets;
}

OutgoingPacket::List Sample::CheckTimeouts()
{
    OutgoingPacket::List needToSend;

    if (_packets.empty()) {
        return needToSend;
    }

    const clock::time_point now = clock::now();
    for (auto it = _packets.begin(); it != _packets.end();) {
        OutgoingPacket&& packet = std::move(it->second);
        if ((now - packet._sendTime) > clock::duration(1000ms)) {

            _timeRangedStats.AddPacket(true, packet._sendTime);

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
    return _packets.empty();
}

TimeRangedStats Sample::GetStats() const
{
    return _timeRangedStats;
}
} // namespace FastTransport::Protocol