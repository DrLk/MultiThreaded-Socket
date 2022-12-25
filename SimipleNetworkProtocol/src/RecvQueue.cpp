#include "RecvQueue.hpp"

namespace FastTransport::Protocol {

RecvQueue::RecvQueue()
    : _queue(QueueSize)
{
}

IPacket::Ptr RecvQueue::AddPacket(IPacket::Ptr&& packet)
{
    const SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();

    if (packetNumber == (std::numeric_limits<SeqNumberType>::max)()) {
        throw std::runtime_error("Wrong packet number");
    }

    if (packetNumber - _beginFullRecievedAck >= QueueSize) {
        return std::move(packet); // drop packet. queue is full
    }

    if (packetNumber < _beginFullRecievedAck) {
        {
            const std::lock_guard lock(_selectiveAcksMutex);
            _selectiveAcks.push_back(packetNumber);
        }

        return std::move(packet);
    }

    {
        const std::lock_guard lock(_selectiveAcksMutex);
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

    {
        const std::lock_guard lock(_data._mutex);
        _data.splice(std::move(data));
    }
}

IPacket::List RecvQueue::GetUserData()
{
    IPacket::List data;

    {
        const std::lock_guard lock(_data._mutex);
        data.splice(std::move(_data));
    }

    return data;
}

std::list<SeqNumberType> RecvQueue::GetSelectiveAcks()
{
    std::list<SeqNumberType> selectiveAcks;

    {
        const std::lock_guard lock(_selectiveAcksMutex);
        selectiveAcks.swap(_selectiveAcks);
    }

    return selectiveAcks;
}

[[nodiscard]] SeqNumberType RecvQueue::GetLastAck() const
{
    return _beginFullRecievedAck;
}

} // namespace FastTransport::Protocol
