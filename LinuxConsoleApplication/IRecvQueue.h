#pragma once

#include <vector>
#include <unordered_map>
#include <memory>

#include "BufferOwner.h"
#include "HeaderBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {

        class IRecvQueue
        {
        public:
            virtual ~IRecvQueue() { }
            virtual void AddPacket(BufferOwner::Ptr& packet) = 0;
            virtual std::vector<char> GetUserData(std::size_t size) = 0;
            virtual std::vector<SeqNumberType> GetSelectiveAcks() = 0;

        public:
        };

        class RecvQueue : public IRecvQueue
        {
        public:
            RecvQueue() : _nextFullRecievedAck(0)
            {

            }

            virtual void AddPacket(BufferOwner::Ptr& packet) override
            {
                SeqNumberType packetNumber = packet->GetHeader().GetSeqNumber();
                if ((_nextFullRecievedAck) == packetNumber)
                {
                    _nextFullRecievedAck++;
                }

                _selectiveAcks.push_back(packetNumber);
                _queue[packetNumber] = packet;
            }

            void ProccessUnorderedPackets()
            {
                while (_queue.find(_nextFullRecievedAck++) != _queue.end());
            }

            virtual std::vector<char> GetUserData(std::size_t needed) override
            {
                std::vector<char> result;
                result.reserve(needed);

                for (auto& value : _queue)
                {
                    const BufferOwner::Ptr& packet = value.second;
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

            virtual std::vector<SeqNumberType> GetSelectiveAcks() override
            {
                return _selectiveAcks;
            }

            std::unordered_map<SeqNumberType, BufferOwner::Ptr> _queue;
            std::vector<SeqNumberType> _selectiveAcks;
            SeqNumberType _nextFullRecievedAck;

        };
    }
}
