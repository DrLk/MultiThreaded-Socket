#pragma once

#include <cstring>
#include <filesystem>
#include <string>

#include "Concepts.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

class MessageReader final {
public:
    MessageReader() = default;
    explicit MessageReader(IPacket::List&& packets);

    MessageReader& read(std::byte* data, std::size_t size);

    template <trivial T>
    MessageReader& operator>>(T& trivial); // NOLINT(fuchsia-overloaded-operator)
    MessageReader& operator>>(IPacket::List& packets); // NOLINT(fuchsia-overloaded-operator)

    template <class T>
    MessageReader& operator>>(std::basic_string<T>& string); // NOLINT(fuchsia-overloaded-operator)
    MessageReader& operator>>(std::filesystem::path& path); // NOLINT(fuchsia-overloaded-operator)
                                                            //
    template <class T>
    MessageReader& operator>>(std::span<T> span); // NOLINT(fuchsia-overloaded-operator)
    
    [[nodiscard]] IPacket::List GetPackets();
    [[nodiscard]] IPacket::List GetFreePackets();

private:
    IPacket& GetPacket();
    void GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::size_t _offset { 4 };
};

template <trivial T>
MessageReader& MessageReader::operator>>(T& trivial) // NOLINT(fuchsia-overloaded-operator)
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

template <class T>
MessageReader& MessageReader::operator>>(std::basic_string<T>& string) // NOLINT(fuchsia-overloaded-operator)
{
    std::uint32_t size = 0;
    read(reinterpret_cast<std::byte*>(&size), sizeof(size)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    string.resize(size);
    read(reinterpret_cast<std::byte*>(string.data()), size * sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    return *this;
}

template <class T>
MessageReader& MessageReader::operator>>(std::span<T> span) // NOLINT(fuchsia-overloaded-operator)
{
    read(reinterpret_cast<std::byte*>(span.data()), span.size() * sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return *this;
}

} // namespace FastTransport::Protocol
