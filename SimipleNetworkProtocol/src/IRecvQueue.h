#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "HeaderBuffer.h"
#include "LockedList.h"

namespace FastTransport
{
    namespace Protocol
    {

        class IRecvQueue
        {
        public:
            virtual ~IRecvQueue() { }
            virtual void AddPacket(std::unique_ptr<IPacket>&& packet) = 0;
            virtual std::list<std::unique_ptr<IPacket>> GetUserData() = 0;
            virtual std::list<SeqNumberType>&& GetSelectiveAcks() = 0;

        public:
        };

        class RecvQueue : public IRecvQueue
        {
        public:
            RecvQueue() : _beginFullRecievedAck(0), _queue(QUEUE_SIZE)
            {

            }

            virtual void AddPacket(std::unique_ptr<IPacket>&& packet) override
            {
                SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();

                if (packetNumber == (std::numeric_limits<SeqNumberType>::max)())
                    throw std::runtime_error("Wrong packet number");

                if (packetNumber - _beginFullRecievedAck >= QUEUE_SIZE)
                    return; //drop packet. queue is full

                _selectiveAcks.push_back(packetNumber);
                _queue[(packetNumber) % QUEUE_SIZE] = std::move(packet);
            }

            void ProccessUnorderedPackets()
            {
                std::list<std::unique_ptr<IPacket>> data;
                while (true)
                {
                    auto& nextPacket = _queue[_beginFullRecievedAck++ % QUEUE_SIZE];
                    if (!nextPacket)
                        break;

                    data.push_back(std::move(nextPacket));
                }

                _beginFullRecievedAck--;

                {
                    std::lock_guard lock(_data._mutex);
                    _data.splice(_data.end(), std::move(data));
                }
            }

            virtual std::list<std::unique_ptr<IPacket>> GetUserData() override
            {
                std::list<std::unique_ptr<IPacket>> data;

                {
                    std::lock_guard lock(_data._mutex);
                    data.splice(data.end(), std::move(_data));
                }

                return data;
            }

            virtual std::list<SeqNumberType>&& GetSelectiveAcks() override
            {
                std::lock_guard lock(_selectiveAcks._mutex);
                return std::move(_selectiveAcks);
            }

            SeqNumberType GetLastAck() const
            {
                return _beginFullRecievedAck;
            }

            const int QUEUE_SIZE = 10;
            std::vector<std::unique_ptr<IPacket>> _queue;
            LockedList< std::unique_ptr<IPacket>> _data;
            LockedList<SeqNumberType> _selectiveAcks;
            SeqNumberType _beginFullRecievedAck;
            SeqNumberType _lastNumberReaded;
        };
    }
}
