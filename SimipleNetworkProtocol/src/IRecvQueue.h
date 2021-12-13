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
            virtual IPacket::Ptr AddPacket(IPacket::Ptr&& packet) = 0;
            virtual std::list<IPacket::Ptr> GetUserData() = 0;
            virtual std::list<SeqNumberType> GetSelectiveAcks() = 0;

        public:
        };

        class RecvQueue : public IRecvQueue
        {
        public:
            RecvQueue() : _beginFullRecievedAck(0), _queue(QUEUE_SIZE)
            {

            }

            virtual IPacket::Ptr AddPacket(IPacket::Ptr&& packet) override
            {
                SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();

                if (packetNumber == (std::numeric_limits<SeqNumberType>::max)())
                    throw std::runtime_error("Wrong packet number");

                if (packetNumber - _beginFullRecievedAck >= QUEUE_SIZE)
                    return packet; //drop packet. queue is full

                if (packetNumber < _beginFullRecievedAck)
                    return packet;

                {
                    std::lock_guard lock(_selectiveAcks._mutex);
                    _selectiveAcks.push_back(packetNumber);
                }

                _queue[(packetNumber) % QUEUE_SIZE] = std::move(packet);

                return nullptr;
            }

            void ProccessUnorderedPackets()
            {
                std::list<IPacket::Ptr> data;
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

            virtual std::list<IPacket::Ptr> GetUserData() override
            {
                std::list<IPacket::Ptr> data;

                {
                    std::lock_guard lock(_data._mutex);
                    data.splice(data.end(), std::move(_data));
                }

                return data;
            }

            virtual std::list<SeqNumberType> GetSelectiveAcks() override
            {
                std::list<SeqNumberType> selectiveAcks;

                {
                    std::lock_guard lock(_selectiveAcks._mutex);
                    selectiveAcks = std::move(_selectiveAcks);
                }

                return selectiveAcks;
            }

            SeqNumberType GetLastAck() const
            {
                return _beginFullRecievedAck;
            }

            const int QUEUE_SIZE = 10;
            std::vector<IPacket::Ptr> _queue;
            LockedList< IPacket::Ptr> _data;
            LockedList<SeqNumberType> _selectiveAcks;
            SeqNumberType _beginFullRecievedAck;
            SeqNumberType _lastNumberReaded;
        };
    }
}
