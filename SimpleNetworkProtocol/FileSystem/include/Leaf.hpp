#pragma once

#include <atomic>
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

// Lifetime model: Leaf is owned by std::shared_ptr. The parent keeps a
// shared_ptr in its `children` map. The kernel "owns" additional lookup
// references that AddRef/ReleaseRef track via _nlookup; the very first
// AddRef captures _selfPin = shared_from_this(), which keeps the Leaf
// alive even after the parent's RemoveChild has dropped its shared_ptr,
// until the matching forget(s) bring _nlookup back to zero. The last
// ReleaseRef releases _selfPin; whichever shared_ptr is the actual last
// reference (parent's or the pin) destroys the Leaf.
class Leaf : public std::enable_shared_from_this<Leaf> {
    using FilePtr = std::unique_ptr<File>;
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;

public:
    Leaf(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size, Leaf* parent);
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that) = delete;
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that) = delete;
    ~Leaf();

    const std::filesystem::path& GetName() const;
    std::filesystem::file_type GetType() const;
    std::uintmax_t GetSize() const;
    bool IsDeleted() const;
    Leaf& AddChild(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size);
    Leaf& AddChild(std::shared_ptr<Leaf> leaf);
    void RemoveChild(const std::string& name);

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    void Rescan();

    const std::map<std::string, std::shared_ptr<Leaf>>& GetChildren() const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;
    // Non-const lookup — returns nullptr when not found. Returns the same
    // shared_ptr that lives in the parent's `children` map, so the caller can
    // keep the Leaf alive past a later RemoveChild that drops the map entry.
    // Structural changes still go through AddChild/RemoveChild so the
    // FileTree index stays consistent.
    std::shared_ptr<Leaf> FindChild(const std::string& name);

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

    [[nodiscard]] std::uint64_t GetNlookup() const noexcept
    {
        return _nlookup.load(std::memory_order_relaxed);
    }

    // Drop _selfPin on this Leaf and every descendant. Called from ~FileTree
    // as a safety net: in normal flow each kernel forget routes through
    // ReleaseLeafRefJob and drains _selfPin, but if the scheduler is already
    // gone by the time the FUSE worker delivers forget, those releases are
    // dropped and _selfPin would otherwise leak on shutdown.
    void DropPinsRecursively() noexcept;

private:
    std::map<std::string, std::shared_ptr<Leaf>> children;
    std::filesystem::path _name;
    std::filesystem::file_type _type;
    uintmax_t _size;
    Leaf* _parent;
    // Kernel-side reference count. AddRef is called whenever we hand a
    // newly-resolved client inode to the kernel (fuse_reply_entry from a
    // lookup, each direntplus entry in readdirplus); ReleaseRef on the
    // matching forget. _selfPin is a shared_ptr to this Leaf that the very
    // first AddRef captures and the very last ReleaseRef releases — that's
    // what keeps the Leaf alive after the parent's RemoveChild has dropped
    // its shared_ptr but the kernel has not yet sent forget.
    //
    // Single-thread invariant: AddRef/ReleaseRef are only invoked from a
    // _mainQueue worker (the response-job path that handles the FUSE message
    // for this side — server's lookup/forget on the server, the equivalent
    // response-in jobs on the client). The cache invalidation path
    // (InvalidateCacheLeafJob) keeps the Leaf alive via its own shared_ptr
    // instead of touching nlookup, so it never crosses thread boundaries
    // here. _nlookup stays atomic only as a relaxed counter for
    // GetNlookup() observability, not for synchronisation.
    mutable std::atomic<std::uint64_t> _nlookup { 0 };
    mutable std::shared_ptr<const Leaf> _selfPin;
    std::uint64_t _serverInode = 0;
    FileTree* _tree = nullptr;

    std::unordered_map<size_t, std::set<std::shared_ptr<FileCache::Range>, FileCache::RangePtrLess>> _data;
    std::shared_ptr<PiecesStatus> _piecesStatus;
    std::unordered_map<size_t, std::vector<std::unique_ptr<IPendingJob>>> _pendingJobs;
};

} // namespace FastTransport::FileSystem
