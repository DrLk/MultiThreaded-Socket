#include "pch.hpp"
#include "InFlightQueue.hpp"

#include <cstddef>
#include <unordered_map>
#include <utility>

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
std::pair<IPacket::List, IPacket::List> InFlightQueue::AddQueue(OutgoingPacket::List&& packets)
{
    IPacket::List freePackets;
    IPacket::List freeInternalPackets;
    std::unordered_set<SeqNumberType> receivedAcks;
    std::unordered_map<SeqNumberType, OutgoingPacket> queue;

    {
        const std::scoped_lock lock(_receivedAcksMutex);
        receivedAcks.swap(_receivedAcks);
    }

    size_t servicePacketNumber = 0;
    size_t doubleSentPacketNumber = 0;
    for (auto& packet : packets) {
        if (packet.NeedAck()) {
            const SeqNumberType packetNumber = packet.GetPacket()->GetSeqNumber();
            const auto& ack = receivedAcks.find(packetNumber);
            if (ack == receivedAcks.end()) {
                queue[packetNumber] = std::move(packet);
            } else {
                doubleSentPacketNumber++;
                receivedAcks.erase(ack);
                freePackets.push_back(std::move(packet.GetPacket()));
            }
        } else {
            servicePacketNumber++;
            freeInternalPackets.push_back(std::move(packet.GetPacket()));
        }
    }

    _samples.emplace_back(std::move(queue), servicePacketNumber, doubleSentPacketNumber);

    {
        const std::scoped_lock lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return { std::move(freeInternalPackets), std::move(freePackets) };
}

void InFlightQueue::AddAcks(std::span<SeqNumberType> acks)
{
    std::unordered_set<SeqNumberType> receivedAcks(acks.begin(), acks.end());

    //_speedController.AddrecievedAcks(receivedAcks.size());

    {
        const std::scoped_lock lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }
}

IPacket::List InFlightQueue::ProcessAcks()
{
    IPacket::List freePackets;

    std::unordered_set<SeqNumberType> receivedAcks;

    {
        const std::scoped_lock lock(_receivedAcksMutex);
        receivedAcks = std::exchange(_receivedAcks, std::unordered_set<SeqNumberType>());
    }

    for (auto& sample : _samples) {
        freePackets.splice(sample.ProcessAcks(receivedAcks));
    }

    {
        const std::scoped_lock lock(_receivedAcksMutex);
        _receivedAcks.merge(std::move(receivedAcks));
    }

    return freePackets;
}

OutgoingPacket::List InFlightQueue::CheckTimeouts()
{
    OutgoingPacket::List needToSend;

    const clock::duration timeout = _speedController.GetTimeout();
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

std::size_t InFlightQueue::GetNumberPacketToSend()
{
    return _speedController.GetNumberPacketToSend();
}

IPacket::List InFlightQueue::GetAllPackets()
{
    IPacket::List freePackets;
    for (auto& sample : _samples) {
        freePackets.splice(sample.FreePackets());
    }

    return freePackets;
}

} // namespace FastTransport::Protocol
