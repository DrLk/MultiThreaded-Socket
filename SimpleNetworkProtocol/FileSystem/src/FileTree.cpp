#include "FileTree.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <utility>

#include "Leaf.hpp"
#include "NativeFile.hpp"

namespace FastTransport::FileSystem {

FileTree::FileTree(const std::filesystem::path& name, FilePtr&& root)
    : _root(std::make_unique<Leaf>(name, std::filesystem::file_type::directory, nullptr))
{
    _root->SetFile(std::move(root));
}

FileTree::FileTree(FileTree&& that) noexcept = default;

FileTree& FileTree::operator=(FileTree&& that) noexcept = default;

FileTree::~FileTree() = default;

Leaf& FileTree::GetRoot()
{
    return *_root;
}

FileTree FileTree::GetTestFileTree()
{
    FileTree tree(
        "/tmp/test",
        std::make_unique<FastTransport::FileSystem::NativeFile>(
            "/tmp/test",
            0,
            std::filesystem::file_type::directory));

    auto& root = tree.GetRoot();

    auto& folder1 = root.AddFile(
        "folder1",
        FilePtr(new NativeFile {
            "folder1",
            0,
            std::filesystem::file_type::directory }));

    root.Find("folder1");

    folder1.AddFile(
        "file1",
        FilePtr(new NativeFile {
            "file1",
            10L * 1024L * 1024L * 1024L,
            std::filesystem::file_type::regular }));

    auto& folder2 = root.AddFile(
        "folder2",
        FilePtr(new NativeFile {
            "folder2",
            0,
            std::filesystem::file_type::directory }));

    folder2.AddFile(
        "file2",
        FilePtr(new NativeFile {
            "file2",
            2LL * 1024 * 1024,
            std::filesystem::file_type::regular }));

    return tree;
}

void FileTree::Scan(const std::filesystem::path& directoryPath)
{
    Scan(std::move(directoryPath), *_root);
}

void FileTree::Scan(const std::filesystem::path& directoryPath, Leaf& root) // NOLINT(misc-no-recursion)
{
    std::filesystem::directory_iterator itt(directoryPath);

    for (; itt != std::filesystem::directory_iterator(); itt++) {
        const std::filesystem::path& path = itt->path();
        if (std::filesystem::is_regular_file(path)) {
            root.AddFile(
                path,
                FilePtr(new NativeFile {
                    path,
                    std::filesystem::file_size(path),
                    std::filesystem::file_type::regular,
                }));

        } else if (std::filesystem::is_directory(path)) {
            auto& directory = root.AddFile(
                path,
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
