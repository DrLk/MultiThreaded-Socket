#pragma once

#include <cassert>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <unordered_map>

#include "FileCache/Range.hpp"
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

    std::unique_ptr<fuse_bufvec> GetData(fuse_ino_t inode, off_t offset, size_t size);
    FileTree::Data AddData(fuse_ino_t inode, off_t offset, size_t size, Data&& data);

    FreeData GetFreeData(size_t size);

    FileCache::FileCache& GetFileCache();

private:
    LeafPtr _root;
    std::filesystem::path _cacheFolder;

    std::unordered_map<fuse_ino_t, int> _cache;
    FileCachePtr _fileCache;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
