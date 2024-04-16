#pragma once

#include <cstddef>
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

    MessageReader& read(void* data, std::size_t size);

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
    IPacket& GetNextPacket();

    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::size_t _offset { 4 };
};

template <trivial T>
MessageReader& MessageReader::operator>>(T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return read(reinterpret_cast<std::byte*>(&trivial), sizeof(T)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, bugprone-sizeof-expression, bugprone-multi-level-implicit-pointer-conversion)
}

template <class T>
MessageReader& MessageReader::operator>>(std::basic_string<T>& string) // NOLINT(fuchsia-overloaded-operator)
{
    static_assert(sizeof(std::uint64_t) == sizeof(typename std::basic_string<T>::size_type));
    std::uint64_t size = 0;
    operator>>(size);
    string.resize(size);
    read(string.data(), size * sizeof(T));

    return *this;
}

template <class T>
MessageReader& MessageReader::operator>>(std::vector<T>& data) // NOLINT(fuchsia-overloaded-operator)
{
    static_assert(sizeof(std::int64_t) == sizeof(typename std::basic_string<T>::size_type));
    std::uint64_t size = 0;
    operator>>(size);
    data.resize(size);
    read(data.data(), size * sizeof(T));
    return *this;
}

} // namespace FastTransport::Protocol
