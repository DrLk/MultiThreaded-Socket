#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "File.hpp"

struct Leaf {
    std::uint64_t inode;
    Leaf* parrent;
    std::map<std::string_view, std::unique_ptr<Leaf>> children;

    File file;

    Leaf& AddFile(File&& file)
    {
        auto leaf = std::make_unique<Leaf>();
        leaf->file = std::move(file);
        leaf->parrent = this;
        auto [insertedLeaf, result] = children.insert({ leaf->file.name.native(), std::move(leaf) });
        return *insertedLeaf->second;
    }

    std::optional<std::reference_wrapper<Leaf>> Find(const std::string& name)
    {
        auto file = children.find(name);
        if (file != children.end())
            return *file->second;

        return {};
    }

private:
};

class FileTree {
public:
    FileTree(File&& file)
    {
        _root = std::make_unique<Leaf>();
        _root->inode = 0;
        _root->file = std::move(file);
        _root->parrent = nullptr;
    }

    std::unique_ptr<Leaf>& GetRoot()
    {
        return _root;
    }

    void AddOpened(const std::shared_ptr<Leaf>& leaf)
    {
        _openedFiles.insert({ leaf->inode, leaf });
    }

    std::shared_ptr<Leaf>& Find(std::uint64_t inode)
    {
        auto file = _openedFiles.find(inode);

        if (file != _openedFiles.end())
            return file->second;

        throw std::runtime_error("Not implemented");
    }

    void Release(std::uint64_t inode)
    {
        _openedFiles.erase(inode);
    }

    static FileTree GetTestFileTree()
    {
        FileTree tree(File {
            .name = "test",
            .size = 0,
            .type = std::filesystem::file_type::directory });
        auto& root = tree.GetRoot();

        auto& folder1 = root->AddFile(File {
            .name = "folder1",
            .size = 0,
            .type = std::filesystem::file_type::directory });

        root->Find("folder1");

        folder1.AddFile(File {
            .name = "file1",
            .size = 1 * 1024 * 1024,
            .type = std::filesystem::file_type::directory });

        auto& folder2 = root->AddFile(File {
            .name = "folder2",
            .size = 0,
            .type = std::filesystem::file_type::directory });

        folder2.AddFile(File {
            .name = "file2",
            .size = 2 * 1024 * 1024,
            .type = std::filesystem::file_type::directory });

        return tree;
    }

private:
    std::unique_ptr<Leaf> _root;

    std::unordered_map<std::uint64_t, std::shared_ptr<Leaf>> _openedFiles;
};
