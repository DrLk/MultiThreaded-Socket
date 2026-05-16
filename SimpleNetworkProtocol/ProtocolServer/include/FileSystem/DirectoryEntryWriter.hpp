#pragma once

#include <cstdint>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>

#include "Concepts.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class DirectoryEntryWriter final {
public:
    explicit DirectoryEntryWriter(IPacket::List&& packets);

    size_t AddDirectoryEntry(std::string_view name, std::uint64_t inode, std::uint32_t mode, off_t off);
    size_t GetEntryPlusSize(std::string_view name);
    size_t AddDirectoryEntryPlus(std::string_view name, const struct stat* stbuf, off_t off);
    IPacket::List GetWritedPackets();
    IPacket::List GetPackets();

private:
    IPacket::List _packets;
    IPacket::List::Iterator _packet;
    std::size_t _offset { 0 };

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
