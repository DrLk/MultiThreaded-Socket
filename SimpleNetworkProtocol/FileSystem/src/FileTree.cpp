#include "FileTree.hpp"

#include <cassert>
#include <filesystem>
#include <memory>

#include "Leaf.hpp"
#include "Logger.hpp"
#include "NativeFile.hpp"

#define TRACER() LOGGER() << "[FileTree] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

FileTree::FileTree(const std::filesystem::path& name)
    : _root(std::make_unique<Leaf>(name, std::filesystem::file_type::directory, nullptr))
{
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
    FileTree tree("/tmp/test");

    auto& root = tree.GetRoot();

    auto& folder1 = root.AddFile(
        "folder1",
        FilePtr(new NativeFile {
            "folder1",
            0,
            std::filesystem::file_type::directory }));

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

void FileTree::Scan()
{
    Scan(_root->GetFullPath(), *_root);
}

void FileTree::Scan(const std::filesystem::path& directoryPath, Leaf& root) // NOLINT(misc-no-recursion)
{
    try {
        std::filesystem::directory_iterator itt(directoryPath);

        for (; itt != std::filesystem::directory_iterator(); itt++) {
            const std::filesystem::path& path = itt->path();
            if (std::filesystem::is_regular_file(path)) {
                root.AddFile(
                    path.filename(),
                    FilePtr(new NativeFile {
                        path.filename(),
                        std::filesystem::file_size(path),
                        std::filesystem::file_type::regular,
                    }));

            } else if (std::filesystem::is_directory(path)) {
                auto& directory = root.AddFile(
                    path.filename(),
                    FilePtr(new NativeFile {
                        path.filename(),
                        0,
                        std::filesystem::file_type::directory,
                    }));

                Scan(path, directory);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TRACER() << "Error: " << e.what();
        return;
    }
}

} // namespace FastTransport::FileSystem
