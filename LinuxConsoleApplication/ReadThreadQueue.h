#pragma once
#include<mutex>
#include<list>

#include "Packet.h"


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
