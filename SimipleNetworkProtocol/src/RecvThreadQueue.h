#pragma once

#include<mutex>
#include<vector>
#include <memory>
#include <list>

#include "UDPPacket.h"
#include "IPacket.h"

namespace FastTransport::Protocol
{
    class RecvThreadQueue
    {
    public:
        RecvThreadQueue()
        {
        }

        RecvThreadQueue(const RecvThreadQueue&) = delete;
        RecvThreadQueue& operator=(const RecvThreadQueue&) = delete;

        std::list<std::unique_ptr<IPacket>> _recvThreadQueue;

    };
}
