#include "Leaf.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include "File.hpp"

namespace FastTransport::FileSystem {

Leaf::Leaf(const std::filesystem::path& name, std::filesystem::file_type type, Leaf* parent)
    : _name(std::move(name))
    , _type(type)
    , _parent(parent)
{
}

Leaf::Leaf(Leaf&& that) noexcept = default;

Leaf& Leaf::operator=(Leaf&& that) noexcept = default;

Leaf::~Leaf() = default;

Leaf& Leaf::AddChild(const std::filesystem::path& name, std::filesystem::file_type type)
{
    Leaf leaf(name, type, this);
    auto [insertedLeaf, result] = children.insert({ leaf.GetName().native(), std::move(leaf) });
    return insertedLeaf->second;
}

Leaf& Leaf::AddChild(Leaf&& leaf)
{
    auto [insertedLeaf, result] = children.insert({ leaf.GetName().native(), std::move(leaf) });
    return insertedLeaf->second;
}

Leaf& Leaf::AddFile(const std::filesystem::path& name, FilePtr&& file)
{
    Leaf leaf(name, file->GetType(), this);
    leaf._file = std::move(file);
    auto [insertedLeaf, result] = children.insert({ leaf.GetName().native(), std::move(leaf) });
    return insertedLeaf->second;
}

const std::filesystem::path& Leaf::GetName() const
{
    return _name;
}

std::filesystem::file_type Leaf::GetType() const
{
    return _type;
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

const std::map<std::string, Leaf>& Leaf::GetChildren() const
{
    return children;
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
    std::filesystem::path path = GetName();
    for (auto* leaf = _parent; leaf != nullptr; leaf = leaf->_parent) {
        path = leaf->GetName() / path;
    }
    return path;
}

} // namespace FastTransport::FileSystem
