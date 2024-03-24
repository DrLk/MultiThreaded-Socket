#include "FileTree.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <utility>

#include "Leaf.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileSystem {

FileTree::FileTree(FilePtr&& root)
    : _root(std::make_unique<Leaf>())
{
    _root->inode = 0;
    _root->parent = nullptr;

    _root->SetFile(std::move(root));
}

FileTree::FileTree(FileTree&& that) = default;

FileTree::~FileTree() = default;

Leaf& FileTree::GetRoot()
{
    return *_root;
}

void FileTree::AddOpened(const std::shared_ptr<Leaf>& leaf)
{
    _openedFiles.insert({ leaf->inode, leaf });
}

void FileTree::Release(std::uint64_t inode)
{
    _openedFiles.erase(inode);
}

FileTree FileTree::GetTestFileTree()
{
    FileTree tree(
        std::make_unique<FastTransport::FileSystem::NativeFile>(
            "test",
            0,
            std::filesystem::file_type::directory));

    auto& root = tree.GetRoot();

    auto& folder1 = root.AddFile(
        FilePtr(new NativeFile {
            "folder1",
            0,
            std::filesystem::file_type::directory }));

    root.Find("folder1");

    folder1.AddFile(
        FilePtr(new NativeFile {
            "file1",
            1 * 1024 * 1024,
            std::filesystem::file_type::regular }));

    auto& folder2 = root.AddFile(
        FilePtr(new NativeFile {
            "folder2",
            0,
            std::filesystem::file_type::directory }));

    folder2.AddFile(
        FilePtr(new NativeFile {
            "file2",
            2 * 1024 * 1024,
            std::filesystem::file_type::regular }));

    return tree;
}

void FileTree::Scan(const std::filesystem::path& directoryPath)
{
    Scan(std::move(directoryPath), *_root);
}

void FileTree::Scan(const std::filesystem::path& directoryPath, Leaf& root)
{
    std::filesystem::directory_iterator itt(directoryPath);

    for (; itt != std::filesystem::directory_iterator(); itt++) {
        const std::filesystem::path& path = itt->path();
        if (std::filesystem::is_regular_file(path)) {
            root.AddFile(
                FilePtr(new NativeFile {
                    path,
                    std::filesystem::file_size(path),
                    std::filesystem::file_type::regular,
                }));

        } else if (std::filesystem::is_directory(path)) {
            auto& directory = root.AddFile(
                FilePtr(new NativeFile {
                    path,
                    std::filesystem::file_size(path),
                    std::filesystem::file_type::directory,
                }));

            Scan(path, directory);
        }
    }
}

} // namespace FastTransport::FileSystem
