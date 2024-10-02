#include "RecvQueue.hpp"

#include <limits>
#include <list>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "RecvQueueStatus.hpp"

namespace FastTransport::Protocol {

RecvQueue::RecvQueue()
    : _queue(QueueSize)
{
}

std::pair<RecvQueueStatus, IPacket::Ptr> RecvQueue::AddPacket(IPacket::Ptr&& packet)
{
    const SeqNumberType packetNumber = packet->GetSeqNumber();

    if (packetNumber == (std::numeric_limits<SeqNumberType>::max)()) {
        throw std::runtime_error("Wrong packet number");
    }

    if (packetNumber < _beginFullRecievedAck) {
        {
            const std::scoped_lock lock(_selectiveAcksMutex);
            _selectiveAcks.push_back(packetNumber);
        }

        return { RecvQueueStatus::Duplicated, std::move(packet) };
    }

    if (packetNumber - _beginFullRecievedAck >= QueueSize) {
        return { RecvQueueStatus::Full, std::move(packet) };
    }

    {
        const std::scoped_lock lock(_selectiveAcksMutex);
        _selectiveAcks.push_back(packetNumber);
    }

    auto& queuePacket = _queue[(packetNumber) % QueueSize];
    if (queuePacket) {
        return { RecvQueueStatus::Duplicated, std::move(packet) };
    }

    queuePacket = std::move(packet);
    return { RecvQueueStatus::New, nullptr };
}

void RecvQueue::ProccessUnorderedPackets()
{
    IPacket::List data;
    while (true) {
        auto& nextPacket = _queue[_beginFullRecievedAck++ % QueueSize];
        assert(_beginFullRecievedAck != (std::numeric_limits<SeqNumberType>::max)());
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
    return _beginFullRecievedAck - 1;
}

} // namespace FastTransport::Protocol
