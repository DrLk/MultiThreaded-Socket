#pragma once

#include <cstring>

#include "Concepts.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class MessageReader final {
public:
    explicit MessageReader(IPacket::List&& packets);

    MessageReader& read(std::byte* data, std::size_t size);

    template <trivial T>
    MessageReader& operator>>(T& trivial) // NOLINT(fuchsia-overloaded-operator)
    {
        auto readSize = std::min(sizeof(trivial), GetPacket().GetPayload().size() - _offset);
        std::memcpy(&trivial, GetPacket().GetPayload().data() + _offset, readSize);
        _offset += readSize;

        std::ptrdiff_t size = sizeof(trivial) - readSize;
        if (size) {
            _packet++;
            std::memcpy((reinterpret_cast<std::byte*>(&trivial)) + readSize, GetPacket().GetPayload().data(), size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            _offset = size;
        }
        return *this;
    }
    MessageReader& operator>>(IPacket::List& packets); // NOLINT(fuchsia-overloaded-operator)

    [[nodiscard]] IPacket::List GetPackets();
    [[nodiscard]] IPacket::List GetFreePackets();

private:
    IPacket& GetPacket();
    void GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::size_t _offset { 4 };
};
} // namespace FastTransport::Protocol
