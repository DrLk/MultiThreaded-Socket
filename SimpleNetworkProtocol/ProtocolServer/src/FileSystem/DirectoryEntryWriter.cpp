#include "DirectoryEntryWriter.hpp"

#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <linux/fuse.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include "IPacket.hpp"

namespace FastTransport::Protocol {

namespace {
    // FUSE wire format requires every dir entry record to be padded to an 8-byte
    // boundary. Reimplemented locally so we don't have to link libfuse just for
    // fuse_add_direntry / fuse_add_direntry_plus.
    constexpr std::size_t Align8(std::size_t value) noexcept
    {
        return (value + 7U) & ~static_cast<std::size_t>(7);
    }

    constexpr std::size_t DirentHeaderSize = offsetof(struct fuse_dirent, name);
    constexpr std::size_t DirentPlusHeaderSize = offsetof(struct fuse_direntplus, dirent) + offsetof(struct fuse_dirent, name);

    [[nodiscard]] std::size_t DirentSize(std::size_t namelen) noexcept
    {
        return Align8(DirentHeaderSize + namelen);
    }

    [[nodiscard]] std::size_t DirentPlusSize(std::size_t namelen) noexcept
    {
        return Align8(DirentPlusHeaderSize + namelen);
    }

    // Translates POSIX file-mode bits to the 4-bit dir-entry "type" field that
    // libfuse / kernel expect in fuse_dirent::type. Same encoding used by libfuse.
    [[nodiscard]] std::uint32_t TypeFromMode(std::uint32_t mode) noexcept
    {
        return (mode & S_IFMT) >> 12U;
    }

    void WriteDirent(char* buf, std::string_view name, std::uint64_t ino, std::uint32_t type, std::uint64_t off)
    {
        const std::size_t namelen = name.size();
        const std::size_t entlen = DirentHeaderSize + namelen;
        const std::size_t entlen_padded = Align8(entlen);

        struct fuse_dirent dirent {};
        dirent.ino = ino;
        dirent.off = off;
        dirent.namelen = static_cast<std::uint32_t>(namelen);
        dirent.type = type;
        std::memcpy(buf, &dirent, DirentHeaderSize);
        std::memcpy(buf + DirentHeaderSize, name.data(), namelen); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (entlen_padded > entlen) {
            std::memset(buf + entlen, 0, entlen_padded - entlen); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }

    void WriteDirentPlus(char* buf, std::string_view name, const struct stat* stbuf, std::uint64_t off)
    {
        const std::size_t namelen = name.size();
        const std::size_t entlen = DirentPlusHeaderSize + namelen;
        const std::size_t entlen_padded = Align8(entlen);

        struct fuse_direntplus entry {};
        if (stbuf != nullptr) {
            entry.entry_out.nodeid = static_cast<std::uint64_t>(stbuf->st_ino);
            entry.entry_out.generation = 0;
            entry.entry_out.entry_valid = 1;
            entry.entry_out.attr_valid = 1;
            entry.entry_out.entry_valid_nsec = 0;
            entry.entry_out.attr_valid_nsec = 0;
            entry.entry_out.attr.ino = static_cast<std::uint64_t>(stbuf->st_ino);
            entry.entry_out.attr.size = static_cast<std::uint64_t>(stbuf->st_size);
            entry.entry_out.attr.blocks = static_cast<std::uint64_t>(stbuf->st_blocks);
            entry.entry_out.attr.atime = static_cast<std::uint64_t>(stbuf->st_atim.tv_sec);
            entry.entry_out.attr.mtime = static_cast<std::uint64_t>(stbuf->st_mtim.tv_sec);
            entry.entry_out.attr.ctime = static_cast<std::uint64_t>(stbuf->st_ctim.tv_sec);
            entry.entry_out.attr.atimensec = static_cast<std::uint32_t>(stbuf->st_atim.tv_nsec);
            entry.entry_out.attr.mtimensec = static_cast<std::uint32_t>(stbuf->st_mtim.tv_nsec);
            entry.entry_out.attr.ctimensec = static_cast<std::uint32_t>(stbuf->st_ctim.tv_nsec);
            entry.entry_out.attr.mode = stbuf->st_mode;
            entry.entry_out.attr.nlink = stbuf->st_nlink;
            entry.entry_out.attr.uid = stbuf->st_uid;
            entry.entry_out.attr.gid = stbuf->st_gid;
            entry.entry_out.attr.rdev = static_cast<std::uint32_t>(stbuf->st_rdev);
            entry.entry_out.attr.blksize = static_cast<std::uint32_t>(stbuf->st_blksize);
            entry.entry_out.attr.flags = 0;
        }
        entry.dirent.ino = stbuf != nullptr ? static_cast<std::uint64_t>(stbuf->st_ino) : 0;
        entry.dirent.off = off;
        entry.dirent.namelen = static_cast<std::uint32_t>(namelen);
        entry.dirent.type = stbuf != nullptr ? TypeFromMode(stbuf->st_mode) : 0;

        std::memcpy(buf, &entry, DirentPlusHeaderSize);
        std::memcpy(buf + DirentPlusHeaderSize, name.data(), namelen); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (entlen_padded > entlen) {
            std::memset(buf + entlen, 0, entlen_padded - entlen); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
    }
} // namespace

DirectoryEntryWriter::DirectoryEntryWriter(IPacket::List&& packets)
    : _packets(std::move(packets))
    , _packet(_packets.begin())
{
}

size_t DirectoryEntryWriter::AddDirectoryEntry(std::string_view name, std::uint64_t inode, std::uint32_t mode, off_t off)
{
    assert(name.size() <= NAME_MAX);
    const std::size_t entlen_padded = DirentSize(name.size());

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    auto* buf = reinterpret_cast<char*>(GetPacket().GetPayload().subspan(_offset).data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    WriteDirent(buf, name, inode, TypeFromMode(mode), static_cast<std::uint64_t>(off));
    _offset += entlen_padded;
    return entlen_padded;
}

size_t DirectoryEntryWriter::GetEntryPlusSize(std::string_view name)
{
    assert(name.size() <= NAME_MAX);
    return DirentPlusSize(name.size());
}

size_t DirectoryEntryWriter::AddDirectoryEntryPlus(std::string_view name, const struct stat* stbuf, off_t off)
{
    assert(name.size() <= NAME_MAX);
    const std::size_t entlen_padded = DirentPlusSize(name.size());

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    auto* buf = reinterpret_cast<char*>(GetPacket().GetPayload().subspan(_offset).data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    WriteDirentPlus(buf, name, stbuf, static_cast<std::uint64_t>(off));
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
