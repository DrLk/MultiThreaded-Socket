#include "NativeFile.hpp"
#include <cstddef>

#ifdef __linux__
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#endif

#include <cassert>
#include <stdexcept>
#include <vector>

#include "IPacket.hpp"

namespace FastTransport::FileSystem {

NativeFile::NativeFile(const std::filesystem::path& name)
    : _file { open(name.c_str(), ~O_NOFOLLOW) } // NOLINT(hicpp-vararg, cppcoreguidelines-pro-type-vararg)
{
}

void NativeFile::Open()
{
    assert(_file != -1);
}

bool NativeFile::IsOpened() const
{
    return _file != -1;
}

int NativeFile::Close()
{
    assert(_file != -1);
    int result = close(_file); // NOLINT (cppcoreguidelines-pro-type-cstyle-cas
    _file = -1;
    return result;
}

int NativeFile::Stat(struct stat& stat)
{
    assert(_file != -1);
    return fstat(_file, &stat);
}

std::size_t NativeFile::Read(IPacket::List& packets, std::size_t size, off_t offset)
{
#ifdef __linux__
    if (packets.empty()) {
        throw std::runtime_error("No packets to read into");
    }

    const std::size_t blockSize = packets.front()->GetPayload().size();
    int blocks = static_cast<int>(size / blockSize);
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

    std::size_t readed = preadv(_file, iovecs.data(), blocks, offset); // NOLINT (cppcoreguidelines-pro-type-cstyle-cast)
    ++packet;
#endif // __linux__
    return readed;
}

void NativeFile::Write(IPacket::List& packets, size_t size, off_t offset)
{
#ifdef __linux__
    if (packets.empty()) {
        throw std::runtime_error("No packets to read into");
    }
    const std::size_t blockSize = packets.front()->GetPayload().size();
    int blocks = static_cast<int>(size / blockSize);
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

    const ssize_t error = pwritev(_file, iovecs.data(), blocks, offset);
    assert(error != -1);
    ++packet;
#endif // __linux__
}
} // namespace FastTransport::FileSystem
