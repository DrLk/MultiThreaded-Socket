#pragma once

#include <cassert>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <unordered_map>

#include "FileCache/BufferView.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

namespace FileCache {
    class FileCache;
} // namespace FileCache

struct FreeData {
    fuse_ino_t inode;
    off_t offset;
    size_t size;
    Containers::MultiList<std::unique_ptr<Protocol::IPacket>> data;
} __attribute__((aligned(64)));

class File;
class Leaf;

class FileTree {
    using FilePtr = std::unique_ptr<File>;
    using LeafPtr = std::unique_ptr<Leaf>;
    using FileCachePtr = std::unique_ptr<FileCache::FileCache>;

public:
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;

    explicit FileTree(std::filesystem::path&& name, std::filesystem::path&& cacheFolder);
    FileTree(const FileTree& that) = delete;
    FileTree(FileTree&& that) noexcept;
    FileTree& operator=(const FileTree& that) = delete;
    FileTree& operator=(FileTree&& that) noexcept;
    ~FileTree();

    Leaf& GetRoot();
    std::filesystem::path GetCacheFolder() const;

    static FileTree GetTestFileTree();

    void Scan();

    FileCache::BufferView GetData(fuse_ino_t inode, off_t offset, size_t size) const;
    FileTree::Data AddData(fuse_ino_t inode, off_t offset, size_t size, Data&& data);

    FreeData GetFreeData();
    bool NeedsEviction() const;

    static constexpr int MaxCachePackets = 10000;

    FileCache::FileCache& GetFileCache();
    // Cancel all pending jobs across the entire file tree. Must be called after
    // stopping the scheduler (so no new jobs are being added) and before destroying
    // the filesystem session (so fuse_reply_err can still be called).
    void CancelAllPendingJobs();

private:
    LeafPtr _root;
    std::filesystem::path _cacheFolder;

    std::unordered_map<fuse_ino_t, int> _cache;
    int _totalCachedPackets = 0;
    FileCachePtr _fileCache;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
