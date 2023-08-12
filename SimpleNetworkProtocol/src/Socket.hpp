#pragma once

#ifdef WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif
#include <cstring> //memset
#include <span>
#include <stdexcept>

#ifdef __APPLE__
#include <fcntl.h>
#endif

#include "ConnectionAddr.hpp"

namespace FastTransport::Protocol {
class Socket {
public:
    explicit Socket(const ConnectionAddr& address)
        : _address(address)
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
        // create a UDP socket
        _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (_socket == INVALID_SOCKET) {
            throw std::runtime_error("socket");
        }

        uint32_t mode = 1; // 1 to enable non-blocking socket
#ifdef WIN32
        ioctlsocket(_socket, FIONBIO, &mode); // NOLINT
#elif __APPLE__
        fcntl(_socket, F_SETFL, O_NONBLOCK); // NOLINT
#else
        ioctl(_socket, FIONBIO, &mode); // NOLINT
#endif

        const int bufferSize = 10 * 1024 * 1024;
        int result = setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        if (result != 0) {
            throw std::runtime_error("Socket: failed to set SO_RCVBUF");
        }

        result = setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        if (result != 0) {
            throw std::runtime_error("Socket: failed to set SO_SNDBUF");
        }

        // bind socket to port
        if (bind(_socket, reinterpret_cast<const sockaddr*>(&_address.GetAddr()), sizeof(sockaddr)) != 0) { // NOLINT
            throw std::runtime_error("Socket: failed to bind");
        }
    }

    [[nodiscard]] int SendTo(std::span<const unsigned char> buffer, const ConnectionAddr& addr) const
    {
        return sendto(_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr.GetAddr()), sizeof(sockaddr_in)); // NOLINT
    }

    int RecvFrom(std::span<unsigned char> buffer, ConnectionAddr& connectionAddr) const
    {
        socklen_t len = sizeof(sockaddr_storage);
        sockaddr_storage addr {};
        int receivedBytes = recvfrom(_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<struct sockaddr*>(&addr), &len); // NOLINT
        connectionAddr = ConnectionAddr(addr);
        return receivedBytes;
    }

    [[nodiscard]] bool WaitRead() const
    {
        fd_set ReadSet {};
        FD_ZERO(&ReadSet); // NOLINT
        FD_SET(_socket, &ReadSet); // NOLINT

        struct timeval timeout {
            .tv_sec = 0, .tv_usec = 100000
        };

        const int result = select(_socket + 1, &ReadSet, nullptr, nullptr, &timeout);

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("Socket: failed to select read");
        }

        if (result == 0) {
            return false;
        }

        return true;
    }

    [[nodiscard]] bool WaitWrite() const
    {
        fd_set WriteSet {};
        FD_ZERO(&WriteSet); // NOLINT
        FD_SET(_socket, &WriteSet); // NOLINT

        struct timeval timeout {
            .tv_sec = 0, .tv_usec = 100000
        };

        const int result = select(_socket + 1, nullptr, &WriteSet, nullptr, &timeout);

        if (result == SOCKET_ERROR) {
            throw std::runtime_error("Socket: failed to select write");
        }

        if (result == 0) {
            return false;
        }

        return true;
    }

private:
#ifdef WIN32
    SOCKET _socket { INVALID_SOCKET };
#else
    int _socket { INVALID_SOCKET };
#endif
    ConnectionAddr _address;
};

} // namespace FastTransport::Protocol
