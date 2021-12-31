#pragma once

#ifdef WIN32
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include <cstring>
#include <stdexcept>
#include <string>

namespace FastTransport::Protocol {

class ConnectionAddr {
public:
    ConnectionAddr()
    {
        std::memset(&_storage, 0, sizeof(_storage));
    };

    explicit ConnectionAddr(const sockaddr_storage& addr)
        : _storage(addr) {};

    ConnectionAddr(const std::string& addr, unsigned short port)
    {
        std::memset(&_storage, 0, sizeof(_storage));
        if (inet_pton(AF_INET, addr.c_str(), &(reinterpret_cast<sockaddr_in*>(&_storage))->sin_addr) != 0) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            _storage.ss_family = AF_INET;
            (reinterpret_cast<sockaddr_in*>(&_storage))->sin_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        } else if (inet_pton(AF_INET6, addr.c_str(), &(reinterpret_cast<sockaddr_in6*>(&_storage))->sin6_addr) != 0) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            _storage.ss_family = AF_INET6;
            (reinterpret_cast<sockaddr_in6*>(&_storage))->sin6_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
    }

    ConnectionAddr(const ConnectionAddr& that) = default;
    ConnectionAddr& operator=(const ConnectionAddr& that) = default;
    ConnectionAddr(ConnectionAddr&& that) = default;
    ConnectionAddr& operator=(ConnectionAddr&& that) = default;
    ~ConnectionAddr() = default;

    bool operator==(const ConnectionAddr& that) const // NOLINT(fuchsia-overloaded-operator)
    {
        return std::memcmp(&_storage, &that._storage, sizeof(sockaddr_in)) == 0;
    }

    void SetPort(unsigned short port)
    {
        (reinterpret_cast<sockaddr_in*>(&_storage))->sin_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    [[nodiscard]] unsigned short GetPort() const
    {
        return ntohs((reinterpret_cast<const sockaddr_in*>(&_storage))->sin_port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    [[nodiscard]] const sockaddr_storage& GetAddr() const
    {
        return _storage;
    }

private:
    sockaddr_storage _storage {};
};

} // namespace FastTransport::Protocol
