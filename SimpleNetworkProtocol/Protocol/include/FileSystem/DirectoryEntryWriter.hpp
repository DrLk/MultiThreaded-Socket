#pragma once

#include <string_view>
#include <sys/stat.h>

#include "Concepts.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class DirectoryEntryWriter final {
public:
    explicit DirectoryEntryWriter(IPacket::List&& packets);

    size_t AddDirectoryEntry(std::string_view name, const struct stat* stbuf, off_t off);
    IPacket::List GetWritedPackets();
    IPacket::List GetPackets();

private:
    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::ptrdiff_t _offset { 0 };

    IPacket& GetPacket();
    IPacket& GetNextPacket();
    DirectoryEntryWriter& write(const void* data, std::size_t size);
    template <trivial T>
    DirectoryEntryWriter& operator<<(const T& trivial); // NOLINT(fuchsia-overloaded-operator)
};


template <trivial T>
DirectoryEntryWriter& DirectoryEntryWriter::operator<<(const T& trivial) // NOLINT(fuchsia-overloaded-operator)
{
    return write(&trivial, sizeof(T)); // NOLINT(bugprone-sizeof-expression, bugprone-multi-level-implicit-pointer-conversion)
}
} // namespace FastTransport::Protocol
