#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"
#include "StreamConcept.hpp"

namespace FastTransport::FileSystem {
struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;

    void Serialize(OutputByteStream& stream) const;

    template <InputStream Stream>
    void Deserialize(InputByteStream<Stream>& stream)
    {
        stream >> name;
        stream >> size;
        stream >> type;
    }
};
} // namespace FastTransport::FileSystem
