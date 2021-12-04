#pragma once

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include<string.h> //memset

namespace FastTransport::Protocol
{
    class Socket
    {
    public:
        Socket(unsigned short port) : _socket(-1), _port(port)
        {

        }

        ~Socket()
        {
#ifdef WIN32
            closesocket(_socket);
#else
            close(_socket);
#endif
        }

        void Init()
        {
            struct sockaddr_in si_me;

            //create a UDP socket
            if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            {
                throw std::runtime_error("socket");
            }

            // zero out the structure
            memset((char*)& si_me, 0, sizeof(si_me));

            si_me.sin_family = AF_INET;
            si_me.sin_port = htons(_port);
            si_me.sin_addr.s_addr = htonl(INADDR_ANY);

            //bind socket to port
            if (bind(_socket, (struct sockaddr*) & si_me, sizeof(si_me)) == -1)
            {
                throw std::runtime_error("bind");
            }

        }

        size_t SendTo(std::vector<char>& buffer, sockaddr* addr, socklen_t len) const
        {
            return sendto(_socket, buffer.data(), buffer.size(), 0, addr, len);
        }

        size_t RecvFrom(std::vector<char>& buffer, sockaddr* addr, socklen_t* len) const
        {
            return recvfrom(_socket, buffer.data(), buffer.size(), 0, addr, len);
        }

    private:
#ifdef WIN32
        SOCKET _socket;
#else
        int _socket;
#endif
        unsigned short _port;
    };

}
