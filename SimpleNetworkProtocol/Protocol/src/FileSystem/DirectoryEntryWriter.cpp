#include "DirectoryEntryWriter.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unistd.h>
#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {
DirectoryEntryWriter::DirectoryEntryWriter(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
}

namespace {

    struct fuse_dirent {
        uint64_t ino;
        uint64_t off;
        uint32_t namelen;
        uint32_t type;
        char name[]; // NOLINT(modernize-avoid-c-arrays, hicpp-avoid-c-arrays, cppcoreguidelines-avoid-c-arrays)
    };

} // namespace

size_t DirectoryEntryWriter::AddDirectoryEntry(std::string_view name, const struct stat* stbuf, off_t off)
{
    const size_t FUSE_NAME_OFFSET = offsetof(struct fuse_dirent, name);
    std::uint32_t namelen = 0;
    size_t entlen = 0;
    size_t entlen_padded = 0;

    namelen = name.size();
    entlen = FUSE_NAME_OFFSET + namelen;
    entlen_padded = (((entlen) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1));

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    operator<<(stbuf->st_ino);
    operator<<(off);
    operator<<(namelen);
    operator<<(static_cast<std::uint32_t>(stbuf->st_mode & S_IFMT >> 12U));
    write(name.data(), name.size());
    std::string zero(entlen_padded - entlen, '\0');
    write(zero.data(), zero.size());
    return entlen_padded;
}

IPacket::List DirectoryEntryWriter::GetWritedPackets()
{
    IPacket::List packets;
    GetNextPacket();
    packets.splice(_packets, _packets.begin(), _packet);
    return packets;
}

IPacket::List DirectoryEntryWriter::GetPackets()
{
    GetPacket().SetPayloadSize(_offset);
    return std::move(_packets);
}

IPacket& DirectoryEntryWriter::GetPacket()
{
    return **_packet;
}

IPacket& DirectoryEntryWriter::GetNextPacket()
{
    GetPacket().SetPayloadSize(_offset);
    _packet++;
    assert(_packet != _packets.end());

    _offset = 0;

    return **_packet;
}

DirectoryEntryWriter& DirectoryEntryWriter::write(const void* data, std::size_t size)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(data); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    while (size > 0) {
        auto& packet = GetPacket();
        auto payload = packet.GetPayload();
        if (_offset == payload.size()) {
            packet = GetNextPacket();
        }

        auto writeSize = std::min<std::uint32_t>(size, packet.GetPayload().size() - _offset);
        std::memcpy(packet.GetPayload().data() + _offset, bytes, writeSize);
        _offset += writeSize;

        size -= writeSize;
        bytes += writeSize; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    return *this;
}

} // namespace FastTransport::Protocol
