#pragma once

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <string.h> //memset
#include <span>

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
            u_long mode = 1;  // 1 to enable non-blocking socket
            ioctlsocket(_socket, FIONBIO, &mode);
        }

        size_t SendTo(std::span<const char> buffer, const sockaddr_storage& addr) const
        {
            return sendto(_socket, buffer.data(), buffer.size(), 0, (sockaddr*) &addr, sizeof(sockaddr_in));
        }

        size_t RecvFrom(std::span<char> buffer, sockaddr_storage& addr) const
        {
            int len = sizeof(sockaddr_storage);
            int receivedBytes = recvfrom(_socket, buffer.data(), buffer.size(), 0, (sockaddr*)&addr, &len);
            return receivedBytes;
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
