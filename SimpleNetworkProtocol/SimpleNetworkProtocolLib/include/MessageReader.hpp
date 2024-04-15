#pragma once

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

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
    MessageReader& operator>>(std::vector<T>& data); // NOLINT(fuchsia-overloaded-operator)
    
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
    const auto readSize = std::min(sizeof(T), GetPacket().GetPayload().size() - _offset); // NOLINT(bugprone-sizeof-expression)
    std::memcpy(&trivial, GetPacket().GetPayload().data() + _offset, readSize); // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
    _offset += readSize;

    const std::ptrdiff_t size = sizeof(T) - readSize; // NOLINT(bugprone-sizeof-expression)
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
    static_assert(sizeof(std::uint64_t) == sizeof(typename std::basic_string<T>::size_type));
    std::uint64_t size = 0;
    operator>>(size);
    string.resize(size);
    read(reinterpret_cast<std::byte*>(string.data()), size * sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    return *this;
}

template <class T>
MessageReader& MessageReader::operator>>(std::vector<T>& data) // NOLINT(fuchsia-overloaded-operator)
{
    static_assert(sizeof(std::int64_t) == sizeof(typename std::basic_string<T>::size_type));
    std::uint64_t size = 0;
    operator>>(size);
    data.resize(size);
    read(reinterpret_cast<std::byte*>(data.data()), size * sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return *this;
}

} // namespace FastTransport::Protocol
