#include "Sample.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {

Sample::Sample(std::unordered_map<SeqNumberType, OutgoingPacket>&& packets)
    : _packets(std::move(packets))
    , _ackPacketNumber(0)
    , _lostPacketNumber(0)
    , _start(0)
    , _end(0)
    , _sendInterval(0)
    , _startTime(0)

{
}

IPacket::List Sample::ProcessAcks(std::unordered_set<SeqNumberType>& acks)
{
    IPacket::List freePackets;
    // for (SeqNumberType number : acks) {
    for (auto ack = acks.begin(); ack != acks.end();) {
        const auto& it = _packets.find(*ack);
        if (it != _packets.end()) {
            freePackets.push_back(std::move(it->second._packet));
            _packets.erase(it);
            ack = acks.erase(ack);

            _ackPacketNumber++;
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

    clock::time_point now = clock::now();
    for (auto it = _packets.begin(); it != _packets.end();) {
        OutgoingPacket&& packet = std::move(it->second);
        if ((now - packet._sendTime) > clock::duration(1000ms)) {
            needToSend.push_back(std::move(packet));
            it = _packets.erase(it);

            _lostPacketNumber++;
        } else {
            it++;
        }
    }

    return needToSend;
}
} // namespace FastTransport::Protocol