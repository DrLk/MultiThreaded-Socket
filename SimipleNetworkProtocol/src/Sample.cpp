#include "Sample.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {

Sample::Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets)
    : _packets(std::move(packets))
    , _ackPacketNumber(0)
    , _lostPacketNumber(0)
    , _startTime(0s)
    , _endTime(0s)
    , _start(0)
    , _end(0)
    , _sendInterval(0)
    , _allPacketsCount(_packets.size())

{
    const auto& [min, max] = std::minmax_element(_packets.begin(), _packets.end(),
        [](const std::pair<const SeqNumberType, OutgoingPacket>& left, const std::pair<const SeqNumberType, OutgoingPacket>& right) {
            return left.second._sendTime < right.second._sendTime;
        });

    _startTime = min->second._sendTime;
    _endTime = max->second._sendTime;
}

IPacket::List Sample::ProcessAcks(std::unordered_set<SeqNumberType>& acks)
{
    IPacket::List freePackets;
    for (auto ack = acks.begin(); ack != acks.end();) {
        const auto& it = _packets.find(*ack);
        if (it != _packets.end()) {

            _timeRangedStats.AddPacket(false, it->second._sendTime);
            _ackPacketNumber++;

            freePackets.push_back(std::move(it->second._packet));
            _packets.erase(it);
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
            _lostPacketNumber++;

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