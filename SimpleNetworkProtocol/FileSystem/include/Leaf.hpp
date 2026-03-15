#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <fuse3/fuse_lowlevel.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include "FileCache/Range.hpp"
#include "PiecesStatus.hpp"
#include "RemoteFileHandle.hpp"

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

struct PendingFuseRequest {
    fuse_req_t request;
    fuse_ino_t inode;
    size_t size;
    off_t offset;
    RemoteFileHandle* remoteFile;
} __attribute__((aligned(64)));

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

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    void Rescan();

    const std::map<std::string, Leaf>& GetChildren() const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;

    std::filesystem::path GetFullPath() const;
    std::filesystem::path GetCachePath() const;

    Data AddData(off_t offset, size_t size, Data&& data);
    std::unique_ptr<fuse_bufvec> GetData(off_t offset, size_t size) const;
    std::pair<off_t, Data> ExtractBlock(size_t index);
    size_t GetFirstBlockIndex() const;
    static constexpr ssize_t BlockSize = static_cast<const size_t>(1000 * 1300U);

    std::shared_ptr<PiecesStatus> GetPiecesStatus();

    bool SetInFlight(size_t blockIndex);
    void AddPendingRequest(size_t blockIndex, PendingFuseRequest req);
    std::vector<PendingFuseRequest> TakePendingRequests(size_t blockIndex);

private:
    std::map<std::string, Leaf> children; // TODO: use std::set
    std::filesystem::path _name;
    std::filesystem::file_type _type;
    uintmax_t _size;
    Leaf* _parent;
    mutable std::uint64_t _nlookup = 0;

    std::unordered_map<size_t, std::set<FileCache::Range>> _data;
    std::shared_ptr<PiecesStatus> _piecesStatus;
    std::unordered_map<size_t, std::vector<PendingFuseRequest>> _pendingRequests;
};

} // namespace FastTransport::FileSystem
