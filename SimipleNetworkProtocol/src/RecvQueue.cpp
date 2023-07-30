#include "RecvQueue.hpp"

namespace FastTransport::Protocol {

RecvQueue::RecvQueue()
    : _queue(QueueSize)
{
}

IPacket::Ptr RecvQueue::AddPacket(IPacket::Ptr&& packet)
{
    const SeqNumberType packetNumber = packet->GetSeqNumber();

    if (packetNumber == (std::numeric_limits<SeqNumberType>::max)()) {
        throw std::runtime_error("Wrong packet number");
    }

    if (packetNumber - _beginFullRecievedAck >= QueueSize) {
        return std::move(packet); // drop packet. queue is full
    }

    if (packetNumber < _beginFullRecievedAck) {
        {
            const std::scoped_lock lock(_selectiveAcksMutex);
            _selectiveAcks.push_back(packetNumber);
        }

        return std::move(packet);
    }

    {
        const std::scoped_lock lock(_selectiveAcksMutex);
        _selectiveAcks.push_back(packetNumber);
    }

    auto& queuePacket = _queue[(packetNumber) % QueueSize];
    if (queuePacket) {
        return std::move(packet);
    }

    queuePacket = std::move(packet);
    return nullptr;
}

void RecvQueue::ProccessUnorderedPackets()
{
    IPacket::List data;
    while (true) {
        auto& nextPacket = _queue[_beginFullRecievedAck++ % QueueSize];
        if (!nextPacket) {
            break;
        }

        data.push_back(std::move(nextPacket));
    }

    _beginFullRecievedAck--;

    if (!data.empty()) {
        _data.LockedSplice(std::move(data));
        _data.NotifyAll();
    }
}

LockedList<IPacket::Ptr>& RecvQueue::GetUserData()
{
    return _data;
}

std::list<SeqNumberType> RecvQueue::GetSelectiveAcks()
{
    std::list<SeqNumberType> selectiveAcks;

    {
        const std::scoped_lock lock(_selectiveAcksMutex);
        selectiveAcks.swap(_selectiveAcks);
    }

    return selectiveAcks;
}

[[nodiscard]] SeqNumberType RecvQueue::GetLastAck() const
{
    return _beginFullRecievedAck;
}

} // namespace FastTransport::Protocol
