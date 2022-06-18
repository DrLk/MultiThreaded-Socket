#pragma once

#ifdef WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <cstring> //memset
#include <span>
#include <stdexcept>

#ifdef __APPLE__
#include <fcntl.h>
#endif

namespace FastTransport::Protocol {
class Socket {
public:
    explicit Socket(unsigned short port)
        : _port(port)
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

    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    void Init()
    {
        struct sockaddr_in si_me {
        };

        // create a UDP socket
        if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            throw std::runtime_error("socket");
        }

        u_long mode = 1; // 1 to enable non-blocking socket
#ifdef WIN32
        ioctlsocket(_socket, FIONBIO, &mode); // NOLINT
#elif __APPLE__
        fcntl(_socket, F_SETFL, O_NONBLOCK); // NOLINT
#else
        ioctl(_socket, FIONBIO, &mode); // NOLINT
#endif

        // zero out the structure
        std::memset(&si_me, 0, sizeof(si_me));

        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(_port);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        // bind socket to port
        if (bind(_socket, reinterpret_cast<const struct sockaddr*>(&si_me), sizeof(si_me)) == -1) { // NOLINT
            throw std::runtime_error("bind");
        }
    }

    [[nodiscard]] int SendTo(std::span<const unsigned char> buffer, const sockaddr_storage& addr) const
    {
        return sendto(_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(sockaddr_in)); // NOLINT
    }

    int RecvFrom(std::span<unsigned char> buffer, sockaddr_storage& addr) const
    {
        socklen_t len = sizeof(sockaddr_storage);
        int receivedBytes = recvfrom(_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<struct sockaddr*>(&addr), &len); // NOLINT
        return receivedBytes;
    }

    void WaitRead() const
    {
        fd_set ReadSet {};
        FD_ZERO(&ReadSet); // NOLINT
        FD_SET(_socket, &ReadSet); // NOLINT
        select(_socket + 1, &ReadSet, nullptr, nullptr, nullptr);
    }

    void WaitWrite() const
    {
        fd_set WriteSet {};
        FD_ZERO(&WriteSet); // NOLINT
        FD_SET(_socket, &WriteSet); // NOLINT
        select(_socket + 1, nullptr, &WriteSet, nullptr, nullptr);
    }

private:
#ifdef WIN32
    SOCKET _socket;
#else
    int _socket { -1 };
#endif
    unsigned short _port;
};

} // namespace FastTransport::Protocol
