#pragma once

#ifdef WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>

#define SOCK_CLOEXEC (0)
#else
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1) // NOLINT(modernize-macro-to-enum)
#endif

#ifdef __APPLE__
#include <fcntl.h>
#endif

#include <cstddef>
#include <span>
#include <stdexcept>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class Socket {
public:
    static constexpr size_t UDPMaxSegments = 64;
    static constexpr int GsoSize = 1400;

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

    void Init();

    [[nodiscard]] int SendTo(std::span<const std::byte> buffer, const ConnectionAddr& addr) const;
    [[nodiscard]] uint32_t SendMsg(IPacket::List& packets) const;

    [[nodiscard]] int RecvFrom(std::span<std::byte> buffer, ConnectionAddr& connectionAddr) const;
    [[nodiscard]] IPacket::List RecvMsg(IPacket::List& packets, size_t index) const;

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
