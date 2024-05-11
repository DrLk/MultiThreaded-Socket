#include "DirectoryEntryWriter.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fuse3/fuse_lowlevel.h>
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

    struct fuse_attr {
        uint64_t ino;
        uint64_t size;
        uint64_t blocks;
        uint64_t atime;
        uint64_t mtime;
        uint64_t ctime;
        uint32_t atimensec;
        uint32_t mtimensec;
        uint32_t ctimensec;
        uint32_t mode;
        uint32_t nlink;
        uint32_t uid;
        uint32_t gid;
        uint32_t rdev;
        uint32_t blksize;
        uint32_t flags;
    } __attribute__((aligned(128)));

    struct fuse_entry_out {
        uint64_t nodeid; /* Inode ID */
        uint64_t generation; /* Inode generation: nodeid:gen must
                                be unique for the fs's lifetime */
        uint64_t entry_valid; /* Cache timeout for the name */
        uint64_t attr_valid; /* Cache timeout for the attributes */
        uint32_t entry_valid_nsec;
        uint32_t attr_valid_nsec;
        struct fuse_attr attr;
    } __attribute__((aligned(128)));

    struct fuse_direntplus {
        struct fuse_entry_out entry_out;
        struct fuse_dirent dirent;
    } __attribute__((aligned(128)));

} // namespace

size_t DirectoryEntryWriter::AddDirectoryEntry(std::string_view name, fuse_ino_t inode, std::uint32_t mode, off_t off)
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

    operator<<(inode);
    operator<<(off);
    operator<<(namelen);
    operator<<(mode & S_IFMT >> 12U);
    write(name.data(), name.size());
    std::string zero(entlen_padded - entlen, '\0');
    write(zero.data(), zero.size());
    return entlen_padded;
}

size_t DirectoryEntryWriter::GetEntryPlusSize(std::string_view name)
{
    constexpr size_t FUSE_NAME_OFFSET_DIRENTPLUS = offsetof(struct fuse_direntplus, dirent.name);
    size_t entryPlusSize = FUSE_NAME_OFFSET_DIRENTPLUS + name.size();
    entryPlusSize = (((entryPlusSize) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1));
    return entryPlusSize;
}

size_t DirectoryEntryWriter::AddDirectoryEntryPlus(std::string_view name, const struct stat* stbuf, off_t off)
{
    constexpr size_t FUSE_NAME_OFFSET_DIRENTPLUS = offsetof(struct fuse_direntplus, dirent.name);
    std::uint32_t namelen = 0;
    size_t entlen = 0;
    size_t entlen_padded = 0;

    namelen = name.size();
    entlen = FUSE_NAME_OFFSET_DIRENTPLUS + namelen;
    entlen_padded = (((entlen) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1));

    if (entlen_padded > GetPacket().GetPayload().size() - _offset) {
        GetPacket().SetPayloadSize(_offset);
        GetNextPacket();
        assert(entlen_padded <= GetPacket().GetPayload().size());
    }

    const uint64_t generation = 0; /* Inode generation: nodeid:gen must
                            be unique for the fs's lifetime */
    const uint64_t entry_valid = 1; /* Cache timeout for the name */
    const uint64_t attr_valid = 1; /* Cache timeout for the attributes */
    const uint32_t entry_valid_nsec = 1;
    const uint32_t attr_valid_nsec = 1;

    const uint64_t ino = stbuf->st_ino;
    const uint64_t size = stbuf->st_size;
    const uint64_t blocks = 0;
    const uint64_t atime = 0;
    const uint64_t mtime = 0;
    const uint64_t ctime = 0;
    const uint32_t atimensec = 0;
    const uint32_t mtimensec = 0;
    const uint32_t ctimensec = 0;
    const auto mode = static_cast<std::uint32_t>(stbuf->st_mode & S_IFMT >> 12U);
    const uint32_t nlink = stbuf->st_nlink;
    const uint32_t uid = stbuf->st_uid;
    const uint32_t gid = stbuf->st_gid;
    const uint32_t rdev = stbuf->st_rdev;
    const uint32_t blksize = stbuf->st_blksize;
    const uint32_t flags = 0;

    operator<<(stbuf->st_ino);
    operator<<(generation);
    operator<<(entry_valid);
    operator<<(attr_valid);
    operator<<(entry_valid_nsec);
    operator<<(attr_valid_nsec);

    operator<<(ino);
    operator<<(size);
    operator<<(blocks);
    operator<<(atime);
    operator<<(mtime);
    operator<<(ctime);
    operator<<(atimensec);
    operator<<(mtimensec);
    operator<<(ctimensec);
    operator<<(mode);
    operator<<(nlink);
    operator<<(uid);
    operator<<(gid);
    operator<<(rdev);
    operator<<(blksize);
    operator<<(flags);

    operator<<(stbuf->st_ino);
    operator<<(off);
    operator<<(namelen);
    operator<<(stbuf->st_mode & S_IFMT >> 12U);
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
