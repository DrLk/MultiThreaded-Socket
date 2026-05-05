#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <functional>
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

    // Eviction is sharded by inode to keep all mutations of a given Leaf on the
    // same cacheTreeQueue worker. Callers pass their own shard index (the one
    // they were dispatched on) and only entries belonging to that shard are
    // considered for eviction.
    FreeData GetFreeData(size_t shardIdx);
    bool NeedsEviction() const;

    static constexpr int MaxCachePackets = 10000;
    static constexpr size_t ShardCount = 4;
    static size_t ShardForInode(fuse_ino_t inode) noexcept
    {
        return std::hash<fuse_ino_t> {}(inode) % ShardCount;
    }

    FileCache::FileCache& GetFileCache();
    // Cancel all pending jobs across the entire file tree. Must be called after
    // stopping the scheduler (so no new jobs are being added) and before destroying
    // the filesystem session (so fuse_reply_err can still be called).
    Leaf* FindLeafByServerInode(std::uint64_t serverInode);

    // Index maintenance hooks invoked by Leaf when the tree is mutated.
    // RegisterLeaf inserts the (serverInode, leaf) pair; UnregisterLeaf removes
    // by the inode the leaf currently reports. Callers must invoke them with
    // matching pre/post values when SetServerInode rekeys an entry.
    void RegisterLeaf(std::uint64_t serverInode, Leaf* leaf);
    void UnregisterLeaf(std::uint64_t serverInode);

    void CancelAllPendingJobs();

private:
    // _serverInodeIndex must outlive _root: when ~FileTree destroys _root,
    // the cascading Leaf destructors (RemoveChild paths) still need a live
    // index to update.
    std::unordered_map<std::uint64_t, Leaf*> _serverInodeIndex;
    LeafPtr _root;
    std::filesystem::path _cacheFolder;

    // Per-shard cache index; each shard is owned exclusively by its
    // corresponding cacheTreeQueue worker, so no cross-shard locking is
    // required. _grandTotal is summed across shards for NeedsEviction().
    std::array<std::unordered_map<fuse_ino_t, int>, ShardCount> _cachePerShard;
    std::atomic<int> _grandTotalCachedPackets { 0 };
    FileCachePtr _fileCache;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
