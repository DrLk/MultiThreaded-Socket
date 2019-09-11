#pragma once

#include<mutex>
#include<vector>
#include <exception>

#include "Packet.h"
#include "LockedList.h"

namespace FastTransport::UDPQueue
{
    class RecvThreadQueue
    {
    public:
        RecvThreadQueue()
        {
        }

        RecvThreadQueue(const RecvThreadQueue&) = delete;
        RecvThreadQueue& operator=(const RecvThreadQueue&) = delete;

        LockedList<Packet*> _recvThreadQueue;

    };
}
