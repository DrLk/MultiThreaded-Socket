#pragma once

#include <Winsock2.h>
#include <vector>

class IPacket
{
public:
    virtual void SetBuffer(std::vector<char> buffer) = 0;
    virtual std::vector<char>& GetBuffer() = 0;

    virtual SOCKADDR_IN& GetDestination() = 0;
};
