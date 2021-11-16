#pragma once

#include "IRecvQueue.h"
#include "ISendQueue.h"

namespace FastTransport
{
    namespace Protocol
    {
        class ISocketQueues
        {
        public:
            virtual ~ISocketQueues() { }
            virtual IRecvQueue* RecvQueue() = 0;
            virtual ISendQueue* SendQueue() = 0;
        };

    }
}
