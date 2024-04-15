
#pragma once

#include <cstddef>
#include <cstdint>
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
    [[nodiscard]] IPacket::List GetDataPackets(std::size_t size);

private:
    IPacket& GetPacket();
    IPacket& GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::uint32_t _writedPacketNumber { 1 };
    std::ptrdiff_t _offset { sizeof(_writedPacketNumber) };
};

template <trivial T>
MessageWriter& MessageWriter::operator<<(const T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return write(&trivial, sizeof(T)); // NOLINT(bugprone-sizeof-expression, bugprone-multi-level-implicit-pointer-conversion)
}

template <class T>
MessageWriter& MessageWriter::operator<<(const std::basic_string<T>& string) // NOLINT(fuchsia-overloaded-operator)
{
    static_assert(sizeof(std::int64_t) == sizeof(typename std::basic_string<T>::size_type));
    operator<<(string.size());
    write(string.c_str(), string.size() * sizeof(T));
    return *this;
}

template <trivial T>
MessageWriter& MessageWriter::operator<<(std::span<T> span) // NOLINT(fuchsia-overloaded-operator)
{
    static_assert(sizeof(std::uint64_t) == sizeof(typename std::span<T>::size_type));
    operator<<(span.size());
    return write(span.data(), span.size() * sizeof(T));
}

} // namespace FastTransport::Protocol
