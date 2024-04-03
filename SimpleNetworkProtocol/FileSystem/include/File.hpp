#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

class File {
    using IPacket = FastTransport::Protocol::IPacket;

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

    virtual void Open()= 0;
    virtual IPacket::List Read(IPacket::List& packets, size_t size, off_t off) = 0;
    virtual void Write(IPacket::List& packets, size_t size, off_t off) = 0;
};

} // namespace FastTransport::FileSystem
