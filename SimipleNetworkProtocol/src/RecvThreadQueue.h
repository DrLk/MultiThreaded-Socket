#pragma once

#include<mutex>
#include<vector>
#include <memory>
#include <list>

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

        IPacket::List _recvThreadQueue;

    };
}
