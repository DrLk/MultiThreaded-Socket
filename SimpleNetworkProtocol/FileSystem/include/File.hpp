#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"

namespace FastTransport::FileSystem {

class File {
public:
    File();
    File(const std::filesystem::path& name, std::uintmax_t size, std::filesystem::file_type type);
    virtual ~File();

    std::filesystem::path name;
    std::uintmax_t size;
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

    virtual void Read() {};
};

} // namespace FastTransport::FileSystem
