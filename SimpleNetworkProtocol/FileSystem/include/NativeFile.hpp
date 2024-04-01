#pragma once

#include "File.hpp"
#include <fuse3/fuse_lowlevel.h>

namespace FastTransport::FileSystem {

static fuse_buf_flags operator|(fuse_buf_flags a, fuse_buf_flags b)
{
    return static_cast<fuse_buf_flags>(static_cast<int>(a) | static_cast<int>(b));
}

class NativeFile : public File {
public:
    NativeFile()
        : NativeFile("", 0, std::filesystem::file_type::none)
    {
    }

    NativeFile(const std::filesystem::path& name, std::uint64_t size, std::filesystem::file_type type)
        : File(name, size, type)
    {
    }

    fuse_bufvec Read(size_t size, off_t off) override
    {
        fuse_bufvec buffer = FUSE_BUFVEC_INIT(size);

        buffer.buf[0].flags = fuse_buf_flags::FUSE_BUF_IS_FD | fuse_buf_flags::FUSE_BUF_FD_SEEK;
        buffer.buf[0].fd = _fd;
        buffer.buf[0].pos = off;
        return buffer;
    };

private:
    int _fd;
};

} // namespace FastTransport::FileSystem
