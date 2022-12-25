#include "InflightQueue.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
IPacket::List InflightQueue::AddQueue(OutgoingPacket::List&& packets)
{
    IPacket::List freePackets;
    std::unordered_set<SeqNumberType> receivedAcks;
    std::unordered_map<SeqNumberType, OutgoingPacket> queue;

    {
        const std::lock_guard lock(_receivedAcksMutex);
        receivedAcks.swap(_receivedAcks);
    }

    for (auto& packet : packets) {
        if (packet._needAck) {
            const SeqNumberType packetNumber = packet._packet->GetHeader().GetSeqNumber();
            const auto& ack = receivedAcks.find(packetNumber);
            if (ack == receivedAcks.end()) {
                queue[packetNumber] = std::move(packet);
            } else {
                receivedAcks.erase(ack);
                freePackets.push_back(std::move(packet._packet));
            }
        } else {
            freePackets.push_back(std::move(packet._packet));
        }
    }

    if (!queue.empty()) {
        _samples.emplace_back(std::move(queue));
    }

    {
        const std::lock_guard lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return freePackets;
}

void InflightQueue::AddAcks(const SelectiveAckBuffer::Acks& acks)
{
    if (!acks.IsValid()) {
        throw std::runtime_error("Not Implemented");
    }

    std::unordered_set<SeqNumberType> receivedAcks(acks.GetAcks().begin(), acks.GetAcks().end());

    //_speedController.AddrecievedAcks(receivedAcks.size());

    {
        const std::lock_guard lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }
}

IPacket::List InflightQueue::ProcessAcks()
{
    IPacket::List freePackets;

    std::unordered_set<SeqNumberType> receivedAcks;

    {
        const std::lock_guard lock(_receivedAcksMutex);
        receivedAcks = std::exchange(_receivedAcks, std::unordered_set<SeqNumberType>());
    }

    for (auto& sample : _samples) {
        freePackets.splice(sample.ProcessAcks(receivedAcks));
    }

    {
        const std::lock_guard lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return freePackets;
}

OutgoingPacket::List InflightQueue::CheckTimeouts()
{
    OutgoingPacket::List needToSend;

    clock::duration const timeout = _speedController.GetTimeout();
    for (auto& sample : _samples) {
        needToSend.splice(sample.CheckTimeouts(timeout));
    }

    _samples.remove_if([this](const auto& sample) {
        if (sample.IsDead()) {
            _speedController.UpdateStats(sample.GetStats());
        }

        return sample.IsDead();
    });

    return needToSend;
}

std::size_t InflightQueue::GetNumberPacketToSend()
{
    return _speedController.GetNumberPacketToSend();
}

} // namespace FastTransport::Protocol
