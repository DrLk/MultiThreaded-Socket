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
    : _file { open(name.c_str(), O_CLOEXEC) } // NOLINT(hicpp-vararg, cppcoreguidelines-pro-type-vararg)
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
NativeFile::IPacket::List NativeFile::Read(IPacket::List& packets, std::size_t size, off_t offset)
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
        ++packet;
    }

    std::size_t readed = preadv(_file, iovecs.data(), blocks, offset); // NOLINT (cppcoreguidelines-pro-type-cstyle-cast)

    if (blockSize * packets.size() != readed) {
        auto readPackets = packets.TryGenerate((readed + blockSize - 1) / blockSize);
        readPackets.back()->SetPayloadSize(readed - ((readPackets.size() -1) * blockSize));
        return readPackets;
    }

#endif // __linux__
    return std::move(packets);
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
        ++packet;
    }

    const ssize_t error = pwritev(_file, iovecs.data(), blocks, offset);
    assert(error != -1);
#endif // __linux__
}
} // namespace FastTransport::FileSystem
