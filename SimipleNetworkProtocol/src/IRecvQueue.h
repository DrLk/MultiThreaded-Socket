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
            virtual std::list<std::unique_ptr<IPacket>> GetUserData(std::size_t size) = 0;
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

                if (packetNumber - _beginFullRecievedAck > QUEUE_SIZE)
                    return; //drop packet. queue is full

                _selectiveAcks.push_back(packetNumber);
                _queue[(packetNumber) % QUEUE_SIZE] = std::move(packet);
            }

            void ProccessUnorderedPackets()
            {
                while (true)
                {
                    auto& nextPacket = _queue[_beginFullRecievedAck++];
                    if (!nextPacket)
                        break;

                    _data.push_back(std::move(nextPacket));
                }

                _beginFullRecievedAck--;
            }

            virtual std::list<std::unique_ptr<IPacket>> GetUserData(std::size_t needed) override
            {
                std::list<std::unique_ptr<IPacket>> data;
                while (true)
                {
                    auto& packet = _queue[_beginFullRecievedAck];
                    if (!packet)
                        return data;

                    data.push_back(std::move(packet));
                    _beginFullRecievedAck++;
                }
                return std::move(data);
            }

            virtual std::list<SeqNumberType>&& GetSelectiveAcks() override
            {
                std::lock_guard<std::mutex> lock(_selectiveAcks._mutex);
                return std::move(_selectiveAcks);
            }

            SeqNumberType GetLastAck() const
            {
                return _beginFullRecievedAck;
            }

            const int QUEUE_SIZE = 10;
            std::vector<std::unique_ptr<IPacket>> _queue;
            std::list < std::unique_ptr<IPacket>> _data;
            LockedList<SeqNumberType> _selectiveAcks;
            SeqNumberType _beginFullRecievedAck;
        };
    }
}
