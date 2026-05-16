#pragma once

#include <filesystem>
#include <list>
#include <memory>
#include <unordered_map>

namespace FastTransport::FileSystem {

class NativeFile;
} // namespace FastTransport::FileSystem

namespace FastTransport::FileSystem::FileCache {

class FileCache {
    static constexpr int MaxFiles = 100;
    static constexpr int CleanupFiles = 50;

public:
    std::shared_ptr<NativeFile> GetFile(const std::filesystem::path& path);

private:
    void CleanupOldFiles();

    std::unordered_map<std::filesystem::path, std::pair<std::shared_ptr<NativeFile>, std::list<std::filesystem::path>::iterator>> _openedFiles;
    std::list<std::filesystem::path> _lruList;
};

} // namespace FastTransport::FileSystem::FileCache
