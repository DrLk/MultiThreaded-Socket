#pragma once

#include<mutex>
#include<list>
#include<vector>
#include <exception>

#include "Packet.h"

namespace FastTransport
{
    class RecvThreadQueue
    {
    public:
        RecvThreadQueue()
        {
        }

        RecvThreadQueue(const RecvThreadQueue&) = delete;
        RecvThreadQueue& operator=(const RecvThreadQueue&) = delete;

        std::mutex _recvThreadQueueMutex;
        std::list<Packet*> _recvThreadQueue;

    };
}
