
#pragma once

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <string>

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

    template <class T>
    MessageWriter& operator<<(const std::basic_string<T>& string); // NOLINT(fuchsia-overloaded-operator)
    MessageWriter& operator<<(const std::filesystem::path& path); // NOLINT(fuchsia-overloaded-operator)

    template <trivial T>
    MessageWriter& operator<<(std::span<T> span); // NOLINT(fuchsia-overloaded-operator)
                                                  //
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

template <class T>
MessageWriter& MessageWriter::operator<<(const std::basic_string<T>& string) // NOLINT(fuchsia-overloaded-operator)
{
    const std::uint32_t size = string.size();
    write(reinterpret_cast<const std::byte*>(&size), sizeof(size)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    write(reinterpret_cast<const std::byte*>(string.c_str()), string.size() * sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return *this;
}

template <trivial T>
MessageWriter& MessageWriter::operator<<(std::span<T> span) // NOLINT(fuchsia-overloaded-operator)
{
    return write(span.data(), span.size() * sizeof(T));
}

} // namespace FastTransport::Protocol
