#include "FileTree.hpp"

#include <cassert>
#include <filesystem>
#include <memory>

#include "FileCache/FileCache.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[FileTree] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

FileTree::FileTree(std::filesystem::path&& name, std::filesystem::path&& cacheFolder)
    : _root(std::make_unique<Leaf>(std::move(name), std::filesystem::file_type::directory, 0, nullptr))
    , _cacheFolder(std::move(cacheFolder))
    , _fileCache(std::make_unique<FileCache::FileCache>())
{
}

FileTree::FileTree(FileTree&& that) noexcept = default;

FileTree& FileTree::operator=(FileTree&& that) noexcept = default;

FileTree::~FileTree() = default;

Leaf& FileTree::GetRoot()
{
    return *_root;
}

std::filesystem::path FileTree::GetCacheFolder() const
{
    return _cacheFolder;
}

FileTree FileTree::GetTestFileTree()
{
    FileTree tree("/tmp/test", "/tmp/test/cache");

    auto& root = tree.GetRoot();

    auto& folder1 = root.AddChild("folder1", std::filesystem::file_type::directory, 0);

    folder1.AddChild("file1", std::filesystem::file_type::regular, 100);

    auto& folder2 = root.AddChild("folder2", std::filesystem::file_type::directory, 0);

    folder2.AddChild("file2", std::filesystem::file_type::regular, 200);
    return tree;
}

void FileTree::Scan()
{
    Scan(_root->GetFullPath(), *_root);
}

std::unique_ptr<fuse_bufvec> FileTree::GetData(fuse_ino_t inode, off_t offset, size_t size)
{
    auto& leaf = inode == FUSE_ROOT_ID ? GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    return leaf.GetData(offset, size);
}

FileTree::Data FileTree::AddData(fuse_ino_t inode, off_t offset, size_t size, Data&& data)
{
    int packetsNumber = static_cast<int>(data.size());
    auto& leaf = inode == FUSE_ROOT_ID ? GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto freePackets = leaf.AddData(offset, size, std::move(data));
    auto entry = _cache.find(inode);
    if (entry != _cache.end()) {
        entry->second += packetsNumber - static_cast<int>(freePackets.size());
    } else {
        _cache.insert({ inode, packetsNumber - freePackets.size() });
    }

    return freePackets;
}

FreeData FileTree::GetFreeData(size_t index)
{
    if (_cache.empty()) {
        return {};
    }

    auto entry = _cache.begin();
    fuse_ino_t inode = entry->first;
    auto& leaf = inode == FUSE_ROOT_ID ? GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto [offset, data] = leaf.ExtractBlock(index);
    entry->second -= static_cast<int>(data.size());
    assert(entry->second >= 0);
    if (entry->second == 0) {
        _cache.erase(entry);
    }
    return { .inode = inode, .offset = offset, .data = std::move(data) };
}

FileCache::FileCache& FileTree::GetFileCache()
{
    return *_fileCache;
}

void FileTree::Scan(const std::filesystem::path& directoryPath, Leaf& root) // NOLINT(misc-no-recursion)
{
    try {
        std::filesystem::directory_iterator itt(directoryPath);

        for (; itt != std::filesystem::directory_iterator(); itt++) {
            const std::filesystem::path& path = itt->path();
            if (std::filesystem::is_regular_file(path)) {
                root.AddChild(path.filename(), std::filesystem::file_type::regular, std::filesystem::file_size(path));

            } else if (std::filesystem::is_directory(path)) {
                auto& directory = root.AddChild(path.filename(), std::filesystem::file_type::directory, 0);
                Scan(path, directory);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TRACER() << "Error: " << e.what();
        return;
    }
}

} // namespace FastTransport::FileSystem
