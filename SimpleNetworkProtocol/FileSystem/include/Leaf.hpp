#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <fuse3/fuse_lowlevel.h>
#include <map>
#include <memory>
#include <string>

#include "FileCache/Range.hpp" 

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

class Leaf {
    using FilePtr = std::unique_ptr<File>;
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;

public:
    Leaf(const std::filesystem::path& name, std::filesystem::file_type type, Leaf* parent);
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that) noexcept;
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that) noexcept;
    ~Leaf();

    const std::filesystem::path& GetName() const;
    std::filesystem::file_type GetType() const;
    std::uintmax_t GetSize() const;
    bool IsDeleted() const;
    Leaf& AddChild(const std::filesystem::path& name, std::filesystem::file_type type);
    Leaf& AddChild(Leaf&& leaf);

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    void Rescan();

    const std::map<std::string, Leaf>& GetChildren() const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;

    std::filesystem::path GetFullPath() const;

    Data AddData(off_t offset, size_t size, Data&& data);
    std::unique_ptr<fuse_bufvec> GetData(off_t offset, size_t size) const;

private:
    std::map<std::string, Leaf> children; // TODO: use std::set
    std::filesystem::path _name;
    std::filesystem::file_type _type;
    Leaf* _parent;
    mutable std::uint64_t _nlookup = 0;

    std::set<FileCache::Range> _data;
};

} // namespace FastTransport::FileSystem
