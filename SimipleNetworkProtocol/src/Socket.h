#pragma once

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
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
#ifdef WIN32
            ioctlsocket(_socket, FIONBIO, &mode);
#else
            ioctl(_socket, FIONBIO, &mode);
#endif
        }

        size_t SendTo(std::span<const char> buffer, const sockaddr_storage& addr) const
        {
            return sendto(_socket, buffer.data(), buffer.size(), 0, (sockaddr*) &addr, sizeof(sockaddr_in));
        }

        size_t RecvFrom(std::span<char> buffer, sockaddr_storage& addr) const
        {
            socklen_t len = sizeof(sockaddr_storage);
            int receivedBytes = recvfrom(_socket, buffer.data(), buffer.size(), 0, (sockaddr*)&addr, &len);
            return receivedBytes;
        }

        void WaitRead() const
        {
            fd_set ReadSet;
            FD_ZERO(&ReadSet);
            FD_SET(_socket, &ReadSet);
            select(0, &ReadSet, nullptr, nullptr, nullptr);
        }

        void WaitWrite() const
        {
            fd_set WriteSet;
            FD_ZERO(&WriteSet);
            FD_SET(_socket, &WriteSet);
            select(0, nullptr, &WriteSet, nullptr, nullptr);
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
