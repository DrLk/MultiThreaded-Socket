#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include "FileCache/BufferView.hpp"
#include "FileCache/Range.hpp"
#include "IPendingJob.hpp"
#include "PiecesStatus.hpp"

namespace FastTransport::Containers {
template <class T>
class MultiList;
} // namespace FastTransport::Containers
  //
namespace FastTransport::Protocol {
class IPacket;
} // namespace FastTransport::Protocol
namespace FastTransport::FileSystem {

class File;
class FileTree;

class Leaf {
    using FilePtr = std::unique_ptr<File>;
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;

public:
    Leaf(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size, Leaf* parent);
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that) noexcept;
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that) noexcept;
    ~Leaf();

    const std::filesystem::path& GetName() const;
    std::filesystem::file_type GetType() const;
    std::uintmax_t GetSize() const;
    bool IsDeleted() const;
    Leaf& AddChild(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size);
    Leaf& AddChild(Leaf&& leaf);
    void RemoveChild(const std::string& name);

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    void Rescan();

    const std::map<std::string, Leaf>& GetChildren() const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;
    // Non-const lookup — returns nullptr when not found. Read-only access by name
    // for callers that walk the tree mutably; structural changes still go through
    // AddChild/RemoveChild so the FileTree index stays consistent.
    Leaf* FindChild(const std::string& name);

    std::filesystem::path GetFullPath() const;
    std::filesystem::path GetCachePath() const;

    Data AddData(off_t offset, size_t size, Data&& data);
    FileCache::BufferView GetData(off_t offset, size_t size) const;
    std::pair<off_t, Data> ExtractBlock(size_t index);
    size_t GetFirstBlockIndex() const;
    static constexpr ssize_t BlockSize = static_cast<const size_t>(1000 * 1300U);

    std::shared_ptr<PiecesStatus> GetPiecesStatus();

    bool SetInFlight(size_t blockIndex);
    void AddPendingJob(size_t blockIndex, std::unique_ptr<IPendingJob> job);
    std::vector<std::unique_ptr<IPendingJob>> TakePendingJobs(size_t blockIndex);
    void InvalidateDataCache();
    // Cancel all pending jobs in this leaf and all descendant leaves.
    // Call this before destroying the FUSE session so that held fuse_req_t handles
    // receive an EIO reply instead of being abandoned silently.
    void CancelAllPendingJobs();

    Leaf* GetParent() const noexcept { return _parent; }
    void SetServerInode(std::uint64_t inode);
    // Returns the inode this Leaf has on the remote server.
    // On the server itself _serverInode is never set, so it falls back to
    // reinterpret_cast<uint64_t>(this) — the same value GetINode() would return.
    std::uint64_t GetServerInode() const noexcept
    {
        if (_serverInode != 0) {
            return _serverInode;
        }
        return reinterpret_cast<std::uint64_t>(this); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    }

    void SetTree(FileTree* tree) noexcept { _tree = tree; }

private:
    std::map<std::string, Leaf> children; // TODO: use std::set
    std::filesystem::path _name;
    std::filesystem::file_type _type;
    uintmax_t _size;
    Leaf* _parent;
    mutable std::uint64_t _nlookup = 0;
    std::uint64_t _serverInode = 0;
    FileTree* _tree = nullptr;

    std::unordered_map<size_t, std::set<FileCache::Range>> _data;
    std::shared_ptr<PiecesStatus> _piecesStatus;
    std::unordered_map<size_t, std::vector<std::unique_ptr<IPendingJob>>> _pendingJobs;
};

} // namespace FastTransport::FileSystem
