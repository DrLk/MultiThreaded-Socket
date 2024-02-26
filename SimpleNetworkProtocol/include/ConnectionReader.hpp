#pragma once

#include <stop_token>
#include <thread>
#include <type_traits>

#include "IConnection.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class ConnectionReader final {
public:
    ConnectionReader(std::stop_token stop, IConnection::Ptr  connection);

    ConnectionReader& read(void* data, std::size_t size);
    ConnectionReader& operator>>(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)

private:
    IPacket& GetPacket();
    IPacket::Ptr& GetNextPacket(std::stop_token stop);

    IConnection::Ptr _connection;
    IPacket::List _freePackets;
    IPacket::List _packets;
    IPacket::List::Iterator _packet { _packets.end() };
    std::ptrdiff_t _offset { 0 };
    std::stop_token _stop;
    bool _error = false;
    std::jthread _sendThread;
};
} // namespace FastTransport::Protocol
