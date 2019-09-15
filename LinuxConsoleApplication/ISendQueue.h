#pragma once

#include <unordered_map>
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ISendQueue
        {
        public:
            virtual ~ISendQueue() { }
            virtual void ProcessAcks(const SelectiveAckBuffer::Acks& acks) = 0;
        };

        class SendQueue : public ISendQueue
        {
            typedef std::vector<char> DataType;

        public:

            void SendPacket(DataType data)
            {

            }

            virtual void ProcessAcks(const SelectiveAckBuffer::Acks& acks) override
            {
                for (SeqNumberType number : acks)
                {
                    _inFlightPackets.erase(number);
                }
            }

            void CheckTimeouts()
            {

            }

            std::unordered_map<SeqNumberType, BufferOwner*> _inFlightPackets;

        };
    }
}
