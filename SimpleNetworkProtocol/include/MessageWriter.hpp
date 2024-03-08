
#pragma once

#include <cstddef>
#include <cstring>
#include <stop_token>
#include <type_traits>

#include "IConnection.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

template <class T>
concept trivial = std::is_trivial_v<T>;

class MessageWriter final {
public:
    explicit MessageWriter(IPacket::List&& packets);
    MessageWriter(const MessageWriter&) = delete;
    MessageWriter(MessageWriter&&) noexcept ;
    MessageWriter& operator=(const MessageWriter&) = delete;
    MessageWriter& operator=(MessageWriter&&) noexcept ;
    ~MessageWriter();

    template <trivial T>
    MessageWriter& operator<<(const T& trivial); // NOLINT(fuchsia-overloaded-operator)
    MessageWriter& operator<<(IPacket::List&& packets); // NOLINT(fuchsia-overloaded-operator)

    MessageWriter& write(const void* data, std::size_t size);
    [[nodiscard]] IPacket::List GetPackets();

private:
    IPacket& GetPacket();
    void GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 0 };
};

template <trivial T>
MessageWriter& MessageWriter::operator<<(const T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return write(&trivial, sizeof(trivial));
}

} // namespace FastTransport::Protocol
