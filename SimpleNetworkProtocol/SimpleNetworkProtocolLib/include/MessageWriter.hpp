
#pragma once

#include <cstddef>
#include <cstring>

#include "Concepts.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class MessageWriter final {
public:
    explicit MessageWriter(IPacket::List&& packets);
    MessageWriter(const MessageWriter&) = delete;
    MessageWriter(MessageWriter&&) noexcept;
    MessageWriter& operator=(const MessageWriter&) = delete;
    MessageWriter& operator=(MessageWriter&&) noexcept;
    ~MessageWriter();

    template <trivial T>
    MessageWriter& operator<<(const T& trivial); // NOLINT(fuchsia-overloaded-operator)
    MessageWriter& operator<<(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)

    MessageWriter& write(const void* data, std::size_t size);
    MessageWriter& flush();
    [[nodiscard]] IPacket::List GetPackets();
    [[nodiscard]] IPacket::List GetWritedPackets();

private:
    IPacket& GetPacket();
    void GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::uint32_t _writedPacketNumber { 1 };
    std::ptrdiff_t _offset { sizeof(_writedPacketNumber) };
};

template <trivial T>
MessageWriter& MessageWriter::operator<<(const T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return write(&trivial, sizeof(trivial));
}

} // namespace FastTransport::Protocol
