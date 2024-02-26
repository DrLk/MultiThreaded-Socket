#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"

namespace FastTransport::FileSystem {
struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;

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
} __attribute__((packed)) __attribute__((aligned(64)));
} // namespace FastTransport::FileSystem
