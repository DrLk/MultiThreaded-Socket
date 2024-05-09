#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace FastTransport::FileSystem {

class File;

class Leaf {
    using FilePtr = std::unique_ptr<File>;

public:
    Leaf(const std::filesystem::path& name, std::filesystem::file_type type, Leaf* parent);
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that) noexcept;
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that) noexcept;
    ~Leaf();

    const std::filesystem::path& GetName() const;
    std::filesystem::file_type GetType() const;
    bool IsDeleted() const;
    void SetFile(FilePtr&& file);
    const File& GetFile() const;
    Leaf& AddChild(const std::filesystem::path& name, std::filesystem::file_type type);
    Leaf& AddChild(Leaf&& leaf);
    Leaf& AddFile(const std::filesystem::path& name, FilePtr&& file);

    void AddRef() const;
    void ReleaseRef() const;
    void ReleaseRef(uint64_t nlookup) const;

    void Rescan();

    const std::map<std::string, Leaf>& GetChildren() const;

    std::optional<std::reference_wrapper<const Leaf>> Find(const std::string& name) const;

    std::filesystem::path GetFullPath() const;

private:
    std::map<std::string, Leaf> children; // TODO: use std::set
    std::filesystem::path _name;
    std::filesystem::file_type _type;
    Leaf* _parent;
    FilePtr _file;
    mutable std::uint64_t _nlookup = 0;
};

} // namespace FastTransport::FileSystem
