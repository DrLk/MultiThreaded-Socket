#pragma once

#include<vector>
#include<sys/socket.h>
#include <netinet/in.h>

namespace FastTransport
{
    class Packet
    {
    public:
        Packet(int size = 0) : _data(size)
        {

        }

        std::vector<char>& GetData()
        {
            return _data;
        }
        sockaddr_in& GetAddr()
        {
            return _addr;
        }

    private:
        std::vector<char> _data;
        sockaddr_in _addr;

    };
}
