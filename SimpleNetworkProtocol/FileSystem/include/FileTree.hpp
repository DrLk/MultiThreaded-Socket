#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <memory>
#include <unordered_map>

#include "FileCache/Range.hpp"
#include "IPacket.hpp"

namespace FastTransport::FileSystem {

class CachedEntry {
public:
    CachedEntry(off_t offset, size_t size, fuse_ino_t inode)
        : offset(offset)
        , size(size)
        , inode(inode)
    {
    }

    [[nodiscard]] off_t GetOffset() const
    {
        return offset;
    }

    [[nodiscard]] size_t GetSize() const
    {
        return size;
    }

    [[nodiscard]] fuse_ino_t GetInode() const
    {
        return inode;
    }

    bool operator==(const CachedEntry& other) const // NOLINT(fuchsia-overloaded-operator)
    {
        return offset == other.offset && size == other.size && inode == other.inode;
    }

private:
    off_t offset;
    size_t size;
    fuse_ino_t inode;
};

struct CachedEntryHash {
    std::size_t operator()(const CachedEntry& entry) const // NOLINT(fuchsia-overloaded-operator)
    {
        auto firstHash = std::hash<off_t> {}(entry.GetOffset());
        auto secondHash = std::hash<size_t> {}(entry.GetSize());
        auto thirdHash = std::hash<fuse_ino_t> {}(entry.GetInode());

        auto result = firstHash ^ secondHash ^ thirdHash;
        return result;
    }
};

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

public:
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;

    explicit FileTree(const std::filesystem::path& name);
    FileTree(const FileTree& that) = delete;
    FileTree(FileTree&& that) noexcept;
    FileTree& operator=(const FileTree& that) = delete;
    FileTree& operator=(FileTree&& that) noexcept;
    ~FileTree();

    Leaf& GetRoot();

    static FileTree GetTestFileTree();

    void Scan();

    std::unique_ptr<fuse_bufvec> GetData(fuse_ino_t inode, off_t offset, size_t size);
    FileTree::Data AddData(fuse_ino_t inode, off_t offset, size_t size, Data&& data);

    FreeData GetFreeData(size_t size);

private:
    LeafPtr _root;

    std::unordered_map<fuse_ino_t, size_t> _cache;
    std::unordered_map<std::uint64_t, std::shared_ptr<Leaf>> _openedFiles;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
