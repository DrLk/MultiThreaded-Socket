#pragma once

#include <array>
#include <cstdint>
#include <filesystem>

namespace FastTransport::FileSystem::FileCache {

class FileHash {
public:
    explicit FileHash(std::filesystem::path& path);
    [[nodiscard]] const std::array<uint8_t, 32>& GetHash() const;

private:
    std::array<uint8_t, 32> _hash;
};

} // namespace FastTransport::FileSystem::FileCache
