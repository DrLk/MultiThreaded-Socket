#pragma once

#include <memory>

namespace FastTransport
{
    namespace Protocol
    {
        class BufferOwner;
        class IRecvQueue
        {
        public:
            void ProcessPackets(std::shared_ptr<BufferOwner>& packet)
            {

            }

        };
    }
}