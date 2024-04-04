#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace FastTransport::FileSystem {

class File;
class Leaf;

class FileTree {
    using FilePtr = std::unique_ptr<File>;
    using LeafPtr = std::unique_ptr<Leaf>;

public:
    explicit FileTree(const std::filesystem::path& name, FilePtr&& root);
    FileTree(FileTree&& that) noexcept;
    ~FileTree();

    Leaf& GetRoot();

    void AddOpened(const std::shared_ptr<Leaf>& leaf);

    void Release(std::uint64_t inode);

    static FileTree GetTestFileTree();

    void Scan(const std::filesystem::path& directoryPath);

private:
    LeafPtr _root;
    std::unordered_map<std::uint64_t, std::shared_ptr<Leaf>> _openedFiles;

    void Scan(const std::filesystem::path& directoryPath, Leaf& root);
};

} // namespace FastTransport::FileSystem
