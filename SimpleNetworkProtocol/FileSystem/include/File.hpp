#pragma once

#include <cstdint>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>

#include "ByteStream.hpp"

namespace FastTransport::FileSystem {

class File {
public:
    File();
    File(const std::filesystem::path& name, std::uint64_t size, std::filesystem::file_type type);
    virtual ~File();

    std::filesystem::path name;
    std::uint64_t size;
    std::filesystem::file_type type;

    const std::filesystem::path& GetName() const
    {
        return name;
    }

    template <OutputStream Stream>
    void Serialize(OutputByteStream<Stream>& stream) const
    {
        stream << name;
        stream << size;
        stream << type;
    }

    template <InputStream Stream>
    void Deserialize(InputByteStream<Stream>& stream)
    {
        stream >> name;
        stream >> size;
        stream >> type;
    }

    virtual fuse_bufvec Read(size_t size,  off_t off)
    {
        fuse_bufvec buffer = FUSE_BUFVEC_INIT(size);
        return buffer;
    };

    virtual void Open() {};
};

} // namespace FastTransport::FileSystem
