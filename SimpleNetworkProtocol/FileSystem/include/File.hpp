#pragma once

#include <cstdint>
#include <filesystem>

#include "ByteStream.hpp"

namespace FastTransport::FileSystem {
struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;

    void Serialize(OutputByteStream& stream) const;
    void Deserialize(InputByteStream& stream);
};
} // namespace FastTransport::FileSystem
