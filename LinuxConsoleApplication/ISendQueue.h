#pragma once

#include <map>
#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ISendQueue
        {
            typedef std::vector<char> DataType;

        public:

            void SendPacket(DataType data)
            {

            }

            void ProcessAcks(const SelectiveAckBuffer& buffer)
            {

            }

            void CheckTimeouts()
            {

            }

            //std::map<SeqNumber, BufferOwner*> _inFlightPackets;

        };
    }
}
