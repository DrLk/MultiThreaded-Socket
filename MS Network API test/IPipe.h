#pragma once

#include <list>

#include "IPacket.h"

class IPipe
{
public:
    virtual std::list<IPacket*> Read(std::list<IPacket*> data) = 0;
    virtual std::list<IPacket*> Write(std::list<IPacket*>& data) = 0;

    virtual std::list<IPacket*> CreatePackets(size_t size, size_t count) const = 0;


    virtual void Listen(int port) = 0;
};
