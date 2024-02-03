#pragma once

#include <cstdint>
#include <filesystem>
#include <initializer_list>

namespace FastTransport::FileSystem {
struct File {
    std::filesystem::path name;
    std::uintmax_t size;
    std::filesystem::file_type type;

    void Serialize(std::ostream& stream) const;
    void Deserialize(std::istream& stream);
};
} // namespace FastTransport::FileSystem
