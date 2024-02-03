#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "File.hpp"

namespace FastTransport::FileSystem {

struct Leaf {

public:
    Leaf() = default;
    Leaf(const Leaf& that) = delete;
    Leaf(Leaf&& that) = default;
    Leaf& operator=(const Leaf& that) = delete;
    Leaf& operator=(Leaf&& that) = default;

    std::uint64_t inode;
    Leaf* parent;
    std::map<std::string_view, Leaf> children; // TODO: use std::set

    File file;

    Leaf& AddFile(File&& file)
    {
        Leaf leaf;
        leaf.file = std::move(file);
        leaf.parent = this;
        auto [insertedLeaf, result] = children.insert({ leaf.file.name.native(), std::move(leaf) });
        return insertedLeaf->second;
    }

    void AddRef()
    {
        _nlookup++;
    }

    void ReleaseRef()
    {
        assert(_nlookup > 0);

        _nlookup--;
    }

    void ReleaseRef(uint64_t nlookup)
    {
        assert(_nlookup >= nlookup);

        _nlookup -= nlookup;
    }

    std::optional<std::reference_wrapper<Leaf>> Find(const std::string& name)
    {
        auto file = children.find(name);
        if (file != children.end())
            return file->second;

        return {};
    }

    void Serialize(std::ostream& stream) const
    {
        file.Serialize(stream);
        stream << children.size();
        for (auto& [name, child] : children) {
            child.Serialize(stream);
        }
    }

    void Deserialize(std::istream& stream, Leaf* parent)
    {
        this->parent = parent;
        stream >> file.name;
        stream >> file.size;
        unsigned char type;
        stream >> type;
        file.type = (std::filesystem::file_type)type;
        int count;
        stream >> count;
        for (int i = 0; i < count; i++) {
            Leaf leaf;
            leaf.Deserialize(stream, this);
            children.insert({ leaf.file.name.native(), std::move(leaf) });
        }
    }

private:
    std::uint64_t _nlookup = 0;
    bool _deleted = false;
};

class FileTree {
public:
    FileTree()
    {
        _root.inode = 0;
        _root.parent = nullptr;
    }

    Leaf& GetRoot()
    {
        return _root;
    }

    void SetRoot(File&& file)
    {
        _root.file = std::move(file);
    }

    void Deserialize(std::istream& in)
    {
        _root.Deserialize(in, nullptr);
    }

    void Serialize(const Leaf& root, std::ostream& stream) const
    {
        std::stringstream ss;
        _root.Serialize(stream);
    }

    void AddOpened(const std::shared_ptr<Leaf>& leaf)
    {
        _openedFiles.insert({ leaf->inode, leaf });
    }

    void Release(std::uint64_t inode)
    {
        _openedFiles.erase(inode);
    }

    static FileTree GetTestFileTree()
    {
        FileTree tree;
        tree.SetRoot(
            File {
                .name = "test",
                .size = 0,
                .type = std::filesystem::file_type::directory,
            });
        auto& root = tree.GetRoot();

        auto& folder1 = root.AddFile(File {
            .name = "folder1",
            .size = 0,
            .type = std::filesystem::file_type::directory });

        root.Find("folder1");

        folder1.AddFile(File {
            .name = "file1",
            .size = 1 * 1024 * 1024,
            .type = std::filesystem::file_type::regular });

        auto& folder2 = root.AddFile(File {
            .name = "folder2",
            .size = 0,
            .type = std::filesystem::file_type::directory });

        folder2.AddFile(File {
            .name = "file2",
            .size = 2 * 1024 * 1024,
            .type = std::filesystem::file_type::regular });

        std::ostringstream out;
        tree.Serialize(tree.GetRoot(), out);

        FileTree deserializedTree;

        return tree;
    }

    void Scan(std::filesystem::path directoryPath)
    {
        Scan(directoryPath, _root);
    }

private:
    Leaf _root;
    std::unordered_map<std::uint64_t, std::shared_ptr<Leaf>> _openedFiles;

    void Scan(std::filesystem::path directoryPath, Leaf& root)
    {
        std::filesystem::directory_iterator itt(directoryPath);

        for (; itt != std::filesystem::directory_iterator(); itt++) {
            const std::filesystem::path& path = itt->path();
            if (std::filesystem::is_regular_file(path)) {
                root.AddFile(File {
                    .name = path,
                    .size = std::filesystem::file_size(path),
                    .type = std::filesystem::file_type::regular,
                });

            } else if (std::filesystem::is_directory(path)) {
                auto& directory = root.AddFile(File {
                    .name = path,
                    .size = std::filesystem::file_size(path),
                    .type = std::filesystem::file_type::directory,
                });

                Scan(path, directory);
            }
        }
    }
};
} // namespace FastTransport::FileSystem
