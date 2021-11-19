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
            virtual std::vector<char> GetUserData(std::size_t size) = 0;
            virtual std::list<SeqNumberType>&& GetSelectiveAcks() = 0;

        public:
        };

        class RecvQueue : public IRecvQueue
        {
        public:
            RecvQueue() : _nextFullRecievedAck(0), _firstFullRecievedAck(0)
            {

            }

            virtual void AddPacket(std::unique_ptr<IPacket>&& packet) override
            {
                SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();
                if ((_nextFullRecievedAck) == packetNumber)
                {
                    _nextFullRecievedAck++;
                }

                _selectiveAcks.push_back(packetNumber);
                _queue[packetNumber] = std::move(packet);
            }

            void ProccessUnorderedPackets()
            {
                while (_queue.find(_nextFullRecievedAck++) != _queue.end());
                _nextFullRecievedAck--;
            }

            virtual std::vector<char> GetUserData(std::size_t needed) override
            {
                std::vector<char> result;
                result.reserve(needed);

                for (auto& value : _queue)
                {
                    const std::unique_ptr<IPacket>& packet = value.second;
                    if (needed >= packet->GetPayload().size())
                    {
                        result.insert(result.end(), packet->GetPayload().begin(), packet->GetPayload().end());
                        needed -= packet->GetPayload().size();
                    }
                    else
                    {
                        throw std::runtime_error("Not implemented");
                    }
                }

                return result;
            }

            virtual std::list<SeqNumberType>&& GetSelectiveAcks() override
            {
                std::lock_guard<std::mutex> lock(_selectiveAcks._mutex);
                return std::move(_selectiveAcks);
            }

            std::unordered_map<SeqNumberType, std::unique_ptr<IPacket>> _queue;
            LockedList<SeqNumberType> _selectiveAcks;
            SeqNumberType _nextFullRecievedAck;
            SeqNumberType _firstFullRecievedAck;
        };
    }
}
