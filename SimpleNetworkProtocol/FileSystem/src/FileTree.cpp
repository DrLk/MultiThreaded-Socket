#include "FileTree.hpp"

#include <cassert>
#include <filesystem>
#include <memory>

#include "Leaf.hpp"
#include "Logger.hpp"

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

    auto& folder1 = root.AddChild("folder1", std::filesystem::file_type::directory);

    folder1.AddChild("file1", std::filesystem::file_type::regular);

    auto& folder2 = root.AddChild("folder2", std::filesystem::file_type::directory);

    folder2.AddChild("file2", std::filesystem::file_type::regular);
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
                root.AddChild(path.filename(), std::filesystem::file_type::regular);

            } else if (std::filesystem::is_directory(path)) {
                auto& directory = root.AddChild(path.filename(), std::filesystem::file_type::directory);
                Scan(path, directory);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TRACER() << "Error: " << e.what();
        return;
    }
}

} // namespace FastTransport::FileSystem
