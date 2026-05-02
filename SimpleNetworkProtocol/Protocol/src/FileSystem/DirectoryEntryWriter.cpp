#include "DirectoryEntryWriter.hpp"

#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstring>
#include <fuse3/fuse_lowlevel.h>
#include <string_view>
#include <unistd.h>
#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

namespace {
    // fuse_add_direntry* needs a null-terminated name. NAME_MAX (255 on Linux)
    // covers any single filename component, so we can avoid the per-entry heap
    // allocation a std::string copy would cost on this hot path.
    class NullTerminatedName {
    public:
        explicit NullTerminatedName(std::string_view name)
        {
            assert(name.size() <= NAME_MAX);
            std::memcpy(_buf.data(), name.data(), name.size());
            _buf.at(name.size()) = '\0';
        }
        [[nodiscard]] const char* c_str() const noexcept { return _buf.data(); }

    private:
        std::array<char, NAME_MAX + 1> _buf {};
    };
} // namespace

DirectoryEntryWriter::DirectoryEntryWriter(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
}

size_t DirectoryEntryWriter::AddDirectoryEntry(std::string_view name, fuse_ino_t inode, std::uint32_t mode, off_t off)
{
    struct stat stbuf {};
    stbuf.st_ino = static_cast<ino_t>(inode);
    stbuf.st_mode = mode;

    const NullTerminatedName nameStr(name);
    const size_t entlen_padded = fuse_add_direntry(nullptr, nullptr, 0, nameStr.c_str(), &stbuf, off);

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    auto* buf = reinterpret_cast<char*>(GetPacket().GetPayload().subspan(_offset).data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    fuse_add_direntry(nullptr, buf, entlen_padded, nameStr.c_str(), &stbuf, off);
    _offset += entlen_padded;
    return entlen_padded;
}

size_t DirectoryEntryWriter::GetEntryPlusSize(std::string_view name)
{
    const NullTerminatedName nameStr(name);
    return fuse_add_direntry_plus(nullptr, nullptr, 0, nameStr.c_str(), nullptr, 0);
}

size_t DirectoryEntryWriter::AddDirectoryEntryPlus(std::string_view name, const struct stat* stbuf, off_t off)
{
    fuse_entry_param entry {};
    entry.ino = stbuf->st_ino;
    entry.attr = *stbuf;
    entry.entry_timeout = 1.0;
    entry.attr_timeout = 1.0;

    const NullTerminatedName nameStr(name);
    const size_t entlen_padded = fuse_add_direntry_plus(nullptr, nullptr, 0, nameStr.c_str(), &entry, off);

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    auto* buf = reinterpret_cast<char*>(GetPacket().GetPayload().subspan(_offset).data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    fuse_add_direntry_plus(nullptr, buf, entlen_padded, nameStr.c_str(), &entry, off);
    _offset += entlen_padded;

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
    auto src = std::span(static_cast<const std::byte*>(data), size);
    while (!src.empty()) {
        auto& packet = GetPacket();
        if (_offset == packet.GetPayload().size()) {
            packet = GetNextPacket();
        }

        auto writeSize = std::min(src.size(), packet.GetPayload().size() - _offset);
        std::memcpy(packet.GetPayload().subspan(_offset).data(), src.data(), writeSize);
        _offset += writeSize;
        src = src.subspan(writeSize);
    }

    return *this;
}

} // namespace FastTransport::Protocol
