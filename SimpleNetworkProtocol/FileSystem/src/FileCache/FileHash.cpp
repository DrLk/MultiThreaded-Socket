#include "FileHash.hpp"

#include <array>
#include <cstdint>
#include <filesystem>

namespace FastTransport::FileSystem::FileCache {

FileHash::FileHash([[maybe_unused]] std::filesystem::path& path)
{
}

const std::array<uint8_t, 32>& FileHash::GetHash() const
{
    return _hash;
}

} // namespace FastTransport::FileSystem::FileCache
