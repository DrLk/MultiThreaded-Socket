#include "FileTree.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <sys/types.h>
#include <utility>

#include "FileCache/FileCache.hpp"
#include "Leaf.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[FileTree] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

FileTree::FileTree(std::filesystem::path&& name, std::filesystem::path&& cacheFolder)
    : _root(std::make_shared<Leaf>(std::move(name), std::filesystem::file_type::directory, 0, nullptr))
    , _cacheFolder(std::move(cacheFolder))
    , _fileCache(std::make_unique<FileCache::FileCache>())
{
    _root->SetTree(this);
}

FileTree::FileTree(FileTree&& that) noexcept
    : _serverInodeIndex(std::move(that._serverInodeIndex))
    , _root(std::move(that._root))
    , _cacheFolder(std::move(that._cacheFolder))
    , _cachePerShard(std::move(that._cachePerShard))
    , _grandTotalCachedPackets(that._grandTotalCachedPackets.load())
    , _fileCache(std::move(that._fileCache))
{
    if (_root != nullptr) {
        _root->SetTree(this);
    }
}

FileTree& FileTree::operator=(FileTree&& that) noexcept
{
    if (this == &that) {
        return *this;
    }
    _serverInodeIndex = std::move(that._serverInodeIndex);
    _root = std::move(that._root);
    _cacheFolder = std::move(that._cacheFolder);
    _cachePerShard = std::move(that._cachePerShard);
    _grandTotalCachedPackets = that._grandTotalCachedPackets.load();
    _fileCache = std::move(that._fileCache);
    if (_root != nullptr) {
        _root->SetTree(this);
    }
    return *this;
}

FileTree::~FileTree()
{
    // Safety net for shutdown: if FUSE forgets arrived after the scheduler
    // was already torn down, their ReleaseLeafRefJobs never ran and the
    // matching _selfPin'd Leaves would still hold themselves alive. Walk
    // the tree and force the pins down so the cascading shared_ptr
    // destruction frees everything.
    if (_root != nullptr) {
        _root->DropPinsRecursively();
    }
}

void FileTree::SetRoot(std::shared_ptr<Leaf> root) noexcept
{
    _root = std::move(root);
    if (_root != nullptr) {
        _root->SetTree(this);
    }
}

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

FileCache::BufferView FileTree::GetData(fuse_ino_t inode, off_t offset, size_t size) const
{
    const auto& leaf = inode == FUSE_ROOT_ID ? *_root : *(reinterpret_cast<const Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    return leaf.GetData(offset, size);
}

FileTree::Data FileTree::AddData(fuse_ino_t inode, off_t offset, size_t size, Data&& data)
{
    const int packetsNumber = static_cast<int>(data.size());
    auto& leaf = inode == FUSE_ROOT_ID ? GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    auto freePackets = leaf.AddData(offset, size, std::move(data));
    const int added = packetsNumber - static_cast<int>(freePackets.size());
    auto& shardCache = _cachePerShard.at(ShardForInode(inode));
    auto entry = shardCache.find(inode);
    if (entry != shardCache.end()) {
        entry->second += added;
    } else {
        shardCache.insert({ inode, added });
    }
    _grandTotalCachedPackets.fetch_add(added, std::memory_order_relaxed);

    return freePackets;
}

FreeData FileTree::GetFreeData(size_t shardIdx)
{
    auto& shardCache = _cachePerShard.at(shardIdx);
    if (shardCache.empty()) {
        return {};
    }

    auto entry = shardCache.begin();
    const fuse_ino_t inode = entry->first;
    auto& leaf = inode == FUSE_ROOT_ID ? GetRoot() : *(reinterpret_cast<Leaf*>(inode)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    const size_t blockIndex = leaf.GetFirstBlockIndex();
    if (blockIndex == SIZE_MAX) {
        return {};
    }
    auto [offset, data] = leaf.ExtractBlock(blockIndex);
    const int extracted = static_cast<int>(data.size());
    entry->second -= extracted;
    assert(entry->second >= 0);
    if (entry->second == 0) {
        shardCache.erase(entry);
    }
    _grandTotalCachedPackets.fetch_sub(extracted, std::memory_order_relaxed);

    const size_t blockStart = blockIndex * static_cast<size_t>(Leaf::BlockSize);
    const size_t size = static_cast<size_t>(std::min(Leaf::BlockSize, static_cast<ssize_t>(leaf.GetSize()) - static_cast<ssize_t>(blockStart)));
    return { .inode = inode, .offset = offset, .size = size, .data = std::move(data) };
}

bool FileTree::NeedsEviction() const
{
    return _grandTotalCachedPackets.load(std::memory_order_relaxed) > MaxCachePackets;
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

std::shared_ptr<Leaf> FileTree::FindLeafByServerInode(std::uint64_t serverInode)
{
    if (serverInode == _root->GetServerInode()) {
        return _root;
    }
    auto entry = _serverInodeIndex.find(serverInode);
    if (entry == _serverInodeIndex.end()) {
        return nullptr;
    }
    // weak_ptr.lock() returns nullptr if the Leaf has been destroyed without
    // a matching UnregisterLeaf — defensive against stale index entries.
    return entry->second.lock();
}

void FileTree::RegisterLeaf(std::uint64_t serverInode, const std::shared_ptr<Leaf>& leaf)
{
    _serverInodeIndex[serverInode] = leaf;
}

void FileTree::UnregisterLeaf(std::uint64_t serverInode)
{
    _serverInodeIndex.erase(serverInode);
}

void FileTree::CancelAllPendingJobs()
{
    LOGGER() << "Cancelling pending jobs for all leaves";
    if (_root != nullptr) {
        _root->CancelAllPendingJobs();
    }
}

} // namespace FastTransport::FileSystem
