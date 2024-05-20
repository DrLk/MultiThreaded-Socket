#include "FileHash.hpp"

#include <filesystem>
#include <functional>

namespace FastTransport::FileSystem::FileCache {

FileHash::FileHash(std::filesystem::path& path)
{
    auto hash = std::hash<std::string_view>()(path.string());
}

const std::array<uint8_t, 32>& FileHash::GetHash() const
{
    return _hash;
}

} // namespace FastTransport::FileSystem::FileCache
