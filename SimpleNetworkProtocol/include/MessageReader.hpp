#pragma once

#include <stop_token>

#include "IConnection.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class MessageReader final {
public:
    MessageReader(IPacket::List&&  packets);

    MessageReader& read(void* data, std::size_t size);
    MessageReader& operator>>(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)

private:
    IPacket& GetPacket();
    void GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 4 };
};
} // namespace FastTransport::Protocol
