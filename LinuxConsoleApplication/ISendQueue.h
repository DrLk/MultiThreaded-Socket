#pragma once

#include <map>
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ISendQueue
        {
        public:
            virtual ~ISendQueue() { }
            virtual void ProcessAcks(const SelectiveAckBuffer& buffer) = 0;
        };

        class SendQueue : public ISendQueue
        {
            typedef std::vector<char> DataType;

        public:

            void SendPacket(DataType data)
            {

            }

            virtual void ProcessAcks(const SelectiveAckBuffer& buffer) override
            {

            }

            void CheckTimeouts()
            {

            }

            //std::map<SeqNumber, BufferOwner*> _inFlightPackets;

        };
    }
}
