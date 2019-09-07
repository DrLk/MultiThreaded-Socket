#pragma once

#include "IPacket.h"

class SelectPacket : public IPacket
{
public:
    SelectPacket(size_t size);
    virtual void SetBuffer(std::vector<char> buffer)override;
    virtual std::vector<char>& GetBuffer() override;

    virtual SOCKADDR_IN& GetDestination() override;

private:
    SOCKADDR_STORAGE src;
    SOCKADDR_IN dst;
    //SOCKADDR_STORAGE dst;
    std::vector<char> buffer;

};