#pragma once

#ifdef WIN32
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace FastTransport::Protocol {

class ConnectionAddr {
public:
    ConnectionAddr()
    {
        std::memset(&_storage, 0, sizeof(_storage));
    };

    explicit ConnectionAddr(const sockaddr_storage& addr)
        : _storage(addr) {};

    ConnectionAddr(std::string_view addr, uint16_t port)
    {
        std::memset(&_storage, 0, sizeof(_storage));
        if (inet_pton(AF_INET, addr.data(), &(reinterpret_cast<sockaddr_in*>(&_storage))->sin_addr) != 0) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            _storage.ss_family = AF_INET;
            (reinterpret_cast<sockaddr_in*>(&_storage))->sin_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        } else if (inet_pton(AF_INET6, addr.data(), &(reinterpret_cast<sockaddr_in6*>(&_storage))->sin6_addr) != 0) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            _storage.ss_family = AF_INET6;
            (reinterpret_cast<sockaddr_in6*>(&_storage))->sin6_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }
    }

    ConnectionAddr(const ConnectionAddr& that) = default;
    ConnectionAddr& operator=(const ConnectionAddr& that) = default;
    ConnectionAddr(ConnectionAddr&& that) noexcept = default;
    ConnectionAddr& operator=(ConnectionAddr&& that) noexcept = default;
    ~ConnectionAddr() = default;

    struct HashFunction {
        size_t operator()(const ConnectionAddr& key) const // NOLINT(fuchsia-overloaded-operator)
        {
            return key.GetPort();
        }
    };

    bool operator==(const ConnectionAddr& that) const // NOLINT(fuchsia-overloaded-operator)
    {
        if (_storage.ss_family != that._storage.ss_family) {
            return false;
        }

        if (_storage.ss_family == AF_INET) {
            const sockaddr_in* addr = (reinterpret_cast<const sockaddr_in*>(&_storage)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            const sockaddr_in* thatAddr = (reinterpret_cast<const sockaddr_in*>(&that._storage)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

            if (addr->sin_port != thatAddr->sin_port) {
                return false;
            }

            return std::memcmp(&addr->sin_addr, &thatAddr->sin_addr, sizeof(in_addr)) == 0;
        }

        throw std::runtime_error("Not implemented");
    }

    void SetPort(uint16_t port)
    {
        (reinterpret_cast<sockaddr_in*>(&_storage))->sin_port = htons(port); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    [[nodiscard]] uint16_t GetPort() const
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
