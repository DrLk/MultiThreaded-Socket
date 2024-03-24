#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace FastTransport::FileSystem {

class File;

struct Leaf {
    using FilePtr = std::unique_ptr<File>;

public:
    Leaf();
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that);
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that);
    ~Leaf();

    std::uint64_t inode {};
    Leaf* parent {};
    std::map<std::string, Leaf> children; // TODO: use std::set

    void SetFile(FilePtr&& file);
    const File& GetFile() const;
    Leaf& AddFile(FilePtr&& file);

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;

    std::size_t Read(size_t offset, std::span<std::byte> buffer)
    {
        return 0;
    }

    std::filesystem::path GetFullPath() const;

private:
    FilePtr _file;
    mutable std::uint64_t _nlookup = 0;
    bool _deleted = false;
};

} // namespace FastTransport::FileSystem
