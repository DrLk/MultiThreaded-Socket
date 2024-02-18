#pragma once

#include <stop_token>
#include <thread>
#include <type_traits>

#include "IConnection.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
template <class T>
concept trivial = std::is_trivial_v<T>;

class ConnectionReader final {
public:
    ConnectionReader(std::stop_token stop, const IConnection::Ptr& connection);

    template <trivial T>
    ConnectionReader& operator<<(const T& trivial); // NOLINT(fuchsia-overloaded-operator)
    ConnectionReader& operator<<(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)
    void Flush();

    ConnectionReader& read(void* data, std::size_t size);

private:
    IPacket& GetPacket();
    IPacket::Ptr& GetNextPacket(std::stop_token stop);

    IConnection::Ptr _connection;
    IPacket::List _freePackets;
    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 0 };
    std::stop_token _stop;
    bool _error = false;
    std::jthread _sendThread;
};
} // namespace FastTransport::Protocol
