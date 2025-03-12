#include "FileCache.hpp"

#include "NativeFile.hpp"

namespace FastTransport::FileSystem::FileCache {
std::shared_ptr<NativeFile> FileCache::GetFile(const std::filesystem::path& path)
{
    auto openedFile = _openedFiles.find(path);
    if (openedFile != _openedFiles.end()) {
        _lruList.splice(_lruList.begin(), _lruList, openedFile->second.second);
        return openedFile->second.first;
    }

    if (_openedFiles.size() >= MaxFiles) {
        CleanupOldFiles();
    }

    _lruList.push_front(path);
    auto file = std::make_shared<NativeFile>(path);
    file->Create();
    _openedFiles[path] = { file, _lruList.begin() };
    return file;
}

void FileCache::CleanupOldFiles()
{
    for (int i = 0; i < CleanupFiles && !_lruList.empty(); ++i) {
        auto last = _lruList.back();
        _openedFiles.erase(last);
        _lruList.pop_back();
    }
}

}  // namespace FastTransport::FileSystem::FileCache
