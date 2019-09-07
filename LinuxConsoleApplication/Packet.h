#pragma once

#include<vector>
#include<sys/socket.h>

class Packet
{
public:
    Packet(int size = 0) : _data(size)
    {

    }

    std::vector<char>& GetData() { return _data; }
    sockaddr* GetAddr() { return _addr; }
    void SetAddr(sockaddr* addr) { _addr = addr; }

private:
    std::vector<char> _data;
    sockaddr* _addr;

};
