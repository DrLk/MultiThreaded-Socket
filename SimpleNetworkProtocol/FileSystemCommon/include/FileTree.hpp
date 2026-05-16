#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <unordered_map>

#include "FileCache/BufferView.hpp"
#include "IPacket.hpp"
#include "RemoteFileHandleRegistry.hpp"

namespace FastTransport::FileSystem {

namespace FileCache {
    class FileCache;
} // namespace FileCache

struct FreeData {
    std::uint64_t inode;
    off_t offset;
    size_t size;
    Containers::MultiList<std::unique_ptr<Protocol::IPacket>> data;
} __attribute__((aligned(64)));

class File;
class Leaf;

class FileTree {
    using FilePtr = std::unique_ptr<File>;
    using LeafPtr = std::shared_ptr<Leaf>;
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
    // Replace the root subtree (used by MergeIn after deserialising a remote
    // tree). Caller is responsible for ensuring the index is consistent: any
    // live Leaf* references and serverInodeIndex entries from the old root are
    // about to dangle, so this should only be called during initialisation.
    void SetRoot(std::shared_ptr<Leaf> root) noexcept;
    std::filesystem::path GetCacheFolder() const;

    static FileTree GetTestFileTree();

    void Scan();

    FileCache::BufferView GetData(std::uint64_t inode, off_t offset, size_t size) const;
    FileTree::Data AddData(std::uint64_t inode, off_t offset, size_t size, Data&& data);

    // Eviction is sharded by inode to keep all mutations of a given Leaf on the
    // same cacheTreeQueue worker. Callers pass their own shard index (the one
    // they were dispatched on) and only entries belonging to that shard are
    // considered for eviction.
    FreeData GetFreeData(size_t shardIdx);
    bool NeedsEviction() const;

    static constexpr int MaxCachePackets = 10000;
    static constexpr size_t ShardCount = 4;
    static size_t ShardForInode(std::uint64_t inode) noexcept
    {
        return std::hash<std::uint64_t> {}(inode) % ShardCount;
    }

    FileCache::FileCache& GetFileCache();

    // Per-FileTree registry that owns server-side RemoteFileHandles via
    // shared_ptr. Unused on the client side (client never opens server-side
    // file handles). Kept here rather than on the scheduler because file
    // handles are filesystem-level state — peers of the inode index.
    RemoteFileHandleRegistry& GetRemoteFileHandleRegistry() noexcept { return _remoteFileHandleRegistry; }

    // Returns nullptr if the inode is unknown OR if the corresponding Leaf has
    // already been destroyed (the index holds weak_ptrs, so a stale entry that
    // somehow outlived the Leaf surfaces cleanly as expired weak_ptr instead
    // of becoming a dangling raw pointer).
    std::shared_ptr<Leaf> FindLeafByServerInode(std::uint64_t serverInode);

    // Index maintenance hooks invoked by Leaf when the tree is mutated.
    // RegisterLeaf inserts the (serverInode, leaf) pair; UnregisterLeaf removes
    // by the inode the leaf currently reports. Callers must invoke them with
    // matching pre/post values when SetServerInode rekeys an entry.
    void RegisterLeaf(std::uint64_t serverInode, const std::shared_ptr<Leaf>& leaf);
    void UnregisterLeaf(std::uint64_t serverInode);

    // Cancel all pending jobs across the entire file tree. Must be called after
    // stopping the scheduler (so no new jobs are being added) and before destroying
    // the filesystem session (so fuse_reply_err can still be called).
    void CancelAllPendingJobs();

private:
    // _serverInodeIndex must outlive _root: when ~FileTree destroys _root,
    // the cascading Leaf destructors (RemoveChild paths) still need a live
    // index to update. All mutations and lookups go through _mainQueue
    // (single-threaded), so no locking is required — see CacheTreeJob
    // subclasses, which only operate on shard-affine per-Leaf state.
    // Holds weak_ptrs so a stale entry surfaces as expired-weak_ptr (clean
    // nullptr from FindLeafByServerInode) rather than a dangling raw pointer.
    std::unordered_map<std::uint64_t, std::weak_ptr<Leaf>> _serverInodeIndex;
    LeafPtr _root;
    std::filesystem::path _cacheFolder;

    // Per-shard cache index; each shard is owned exclusively by its
    // corresponding cacheTreeQueue worker, so no cross-shard locking is
    // required. _grandTotal is summed across shards for NeedsEviction().
    std::array<std::unordered_map<std::uint64_t, int>, ShardCount> _cachePerShard;
    std::atomic<int> _grandTotalCachedPackets { 0 };
    FileCachePtr _fileCache;
    RemoteFileHandleRegistry _remoteFileHandleRegistry;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
