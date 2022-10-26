#include "IInFilghtQueue.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
IPacket::List IInflightQueue::AddQueue(OutgoingPacket::List&& packets)
{
    IPacket::List freePackets;
    std::unordered_set<SeqNumberType> receivedAcks;
    std::unordered_map<SeqNumberType, OutgoingPacket> queue;

    {
        std::lock_guard lock(_receivedAcksMutex);
        receivedAcks.swap(_receivedAcks);
    }

    for (auto& packet : packets) {
        if (packet._needAck) {
            SeqNumberType packetNumber = packet._packet->GetHeader().GetSeqNumber();
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
        std::lock_guard lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return freePackets;
}

static int countAcks = 0;
void IInflightQueue::AddAcks(const SelectiveAckBuffer::Acks& acks)
{
    if (!acks.IsValid()) {
        throw std::runtime_error("Not Implemented");
    }

    std::unordered_set<SeqNumberType> receivedAcks(acks.GetAcks().begin(), acks.GetAcks().end());

    //_speedController.AddrecievedAcks(receivedAcks.size());

    {
        std::lock_guard lock(_receivedAcksMutex);
        countAcks += receivedAcks.size();
        _receivedAcks.merge(std::move(receivedAcks));
    }
}

IPacket::List IInflightQueue::ProcessAcks()
{
    IPacket::List freePackets;

    std::unordered_set<SeqNumberType> receivedAcks;

    {
        std::lock_guard lock(_receivedAcksMutex);
        receivedAcks = std::exchange(_receivedAcks, std::unordered_set<SeqNumberType>());
    }

    for (auto& sample : _samples) {
        freePackets.splice(sample.ProcessAcks(receivedAcks));
    }

    {
        std::lock_guard lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return freePackets;
}

OutgoingPacket::List IInflightQueue::CheckTimeouts()
{
    OutgoingPacket::List needToSend;

    for (auto& sample : _samples) {
        needToSend.splice(sample.CheckTimeouts());
    }

    _samples.remove_if([this](const auto& sample) {
        if (sample.IsDead()) {
            _speedController.UpdateStats(sample.GetStats());
        }

        return sample.IsDead();
    });

    return needToSend;
}

size_t IInflightQueue::GetNumberPacketToSend()
{
    return _speedController.GetNumberPacketToSend();
}

} // namespace FastTransport::Protocol
