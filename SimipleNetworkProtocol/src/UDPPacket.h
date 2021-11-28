#pragma once

#include<vector>
#ifdef POSIX
#include<sys/socket.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#endif

namespace FastTransport::Protocol
{
    /*class Packet
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

    };*/
}
