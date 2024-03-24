#include "Leaf.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include "File.hpp"

namespace FastTransport::FileSystem {

Leaf::Leaf() = default;

Leaf::Leaf(Leaf&& that) = default;

Leaf& Leaf::operator=(Leaf&& that) = default;

Leaf::~Leaf() = default;

Leaf& Leaf::AddFile(FilePtr&& file)
{
    Leaf leaf;
    leaf._file = std::move(file);
    leaf.parent = this;
    auto [insertedLeaf, result] = children.insert({ leaf._file->GetName().native(), std::move(leaf) });
    return insertedLeaf->second;
}

void Leaf::SetFile(FilePtr&& file)
{
    _file = std::move(file);
}

const File& Leaf::GetFile() const
{
    return *_file;
}

void Leaf::AddRef() const
{
    _nlookup++;
}

void Leaf::ReleaseRef() const
{
    assert(_nlookup > 0);

    _nlookup--;
}

void Leaf::ReleaseRef(uint64_t nlookup) const
{
    assert(_nlookup >= nlookup);

    _nlookup -= nlookup;
}

std::optional<std::reference_wrapper<const Leaf>> Leaf::Find(const std::string& name) const
{
    auto file = children.find(name);
    if (file != children.end()) {
        return file->second;
    }

    return {};
}

std::filesystem::path Leaf::GetFullPath() const
{
    std::filesystem::path path = GetFile().GetName();
    for (auto* leaf = parent; leaf != nullptr; leaf = leaf->parent) {
        path = leaf->GetFile().GetName() / path;
    }
    return path;
}

} // namespace FastTransport::FileSystem
