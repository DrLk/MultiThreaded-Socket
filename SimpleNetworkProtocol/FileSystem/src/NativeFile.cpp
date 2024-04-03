#include "NativeFile.hpp"

#ifdef __linux__
#include <fcntl.h>
#include <sys/uio.h>
#endif

#include <stdexcept>
#include <vector>

#include "IPacket.hpp"

namespace FastTransport::FileSystem {

NativeFile::NativeFile()
    : NativeFile("", 0, std::filesystem::file_type::none)
{
}

NativeFile::NativeFile(const std::filesystem::path& name, std::uint64_t size, std::filesystem::file_type type)
    : File(name, size, type)
{
}

void NativeFile::Open()
{
    assert(_file != -1);
}

NativeFile::IPacket::List NativeFile::Read(IPacket::List& packets, size_t size, off_t offset)
{
    IPacket::List result;
#ifdef __linux__
    if (packets.empty()) {
        throw std::runtime_error("No packets to read into");
    }
    size_t blockSize = packets.front()->GetPayload().size();
    int blocks = (int)size / blockSize;
    if (size % blockSize != 0) {
        blocks++;
    }

    std::vector<iovec> iovecs(blocks);
    auto packet = packets.begin();
    for (int i = 0; i < blocks; i++) {
        iovecs[i].iov_base = (*packet)->GetPayload().data();
        iovecs[i].iov_len = blockSize;
        assert(blockSize == (*packet)->GetPayload().size());
    }

    int error = preadv(_file, iovecs.data(), blocks, offset);
    assert(error != -1);
    ++packet;
    result.splice(packets, packets.begin(), packet);
#endif // __linux__
    return result;
}

void NativeFile::Write(IPacket::List& packets, size_t size, off_t offset)
{
#ifdef __linux__
    if (packets.empty()) {
        throw std::runtime_error("No packets to read into");
    }
    size_t blockSize = packets.front()->GetPayload().size();
    int blocks = (int)size / blockSize;
    if (size % blockSize != 0) {
        blocks++;
    }

    std::vector<iovec> iovecs(blocks);
    auto packet = packets.begin();
    for (int i = 0; i < blocks; i++) {
        iovecs[i].iov_base = (*packet)->GetPayload().data();
        iovecs[i].iov_len = blockSize;
        assert(blockSize == (*packet)->GetPayload().size());
    }

    int error = pwritev(_file, iovecs.data(), blocks, offset);
    assert(error != -1);
    ++packet;
#endif // __linux__
}
} // namespace FastTransport::FileSystem
