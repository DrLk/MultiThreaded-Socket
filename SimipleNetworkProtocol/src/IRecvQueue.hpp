#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {
class IRecvQueue {
public:
    IRecvQueue() = default;
    IRecvQueue(const IRecvQueue&) = default;
    IRecvQueue(IRecvQueue&&) = default;
    IRecvQueue& operator=(const IRecvQueue&) = default;
    IRecvQueue& operator=(IRecvQueue&&) = default;

    virtual ~IRecvQueue() = default;
    virtual IPacket::Ptr AddPacket(IPacket::Ptr&& packet) = 0;
    virtual IPacket::List GetUserData() = 0;
    virtual std::list<SeqNumberType> GetSelectiveAcks() = 0;
};

class RecvQueue : public IRecvQueue {
public:
    RecvQueue()
        : _queue(Queue_Ssize)
    {
    }

    IPacket::Ptr AddPacket(IPacket::Ptr&& packet) override
    {
        const SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();

        if (packetNumber == (std::numeric_limits<SeqNumberType>::max)()) {
            throw std::runtime_error("Wrong packet number");
        }

        if (packetNumber - _beginFullRecievedAck >= Queue_Ssize) {
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

        auto& queuePacket = _queue[(packetNumber) % Queue_Ssize];
        if (queuePacket) {
            return std::move(packet);
        }

        queuePacket = std::move(packet);
        return nullptr;
    }

    void ProccessUnorderedPackets()
    {
        IPacket::List data;
        while (true) {
            auto& nextPacket = _queue[_beginFullRecievedAck++ % Queue_Ssize];
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

    IPacket::List GetUserData() override
    {
        IPacket::List data;

        {
            const std::lock_guard lock(_data._mutex);
            data.splice(std::move(_data));
        }

        return data;
    }

    std::list<SeqNumberType> GetSelectiveAcks() override
    {
        std::list<SeqNumberType> selectiveAcks;

        {
            const std::lock_guard lock(_selectiveAcksMutex);
            selectiveAcks = std::move(_selectiveAcks);
        }

        return selectiveAcks;
    }

    [[nodiscard]] SeqNumberType GetLastAck() const
    {
        return _beginFullRecievedAck;
    }

private:
    static constexpr int Queue_Ssize = 1000;
    std::vector<IPacket::Ptr> _queue;
    LockedList<IPacket::Ptr> _data;
    std::mutex _selectiveAcksMutex;
    std::list<SeqNumberType> _selectiveAcks;
    SeqNumberType _beginFullRecievedAck { 0 };
};
} // namespace FastTransport::Protocol
