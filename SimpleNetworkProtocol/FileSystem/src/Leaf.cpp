#include "Leaf.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

#include "Logger.hpp"

#define TRACER() LOGGER() << "[Leaf] " // NOLINT(cppcoreguidelines-macro-usage)

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

const std::filesystem::path& Leaf::GetName() const
{
    return _name;
}

std::filesystem::file_type Leaf::GetType() const
{
    return _type;
}

std::uintmax_t Leaf::GetSize() const
{
    if (_type == std::filesystem::file_type::regular) {
        return std::filesystem::file_size(GetFullPath());
    }

    return 0;
}

bool Leaf::IsDeleted() const
{
    return _type == std::filesystem::file_type::not_found;
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

void Leaf::Rescan()
{
    try {
        std::filesystem::directory_iterator itt(GetFullPath());

        for (; itt != std::filesystem::directory_iterator(); itt++) {
            const std::filesystem::path& path = itt->path();

            if (Find(path.filename().native()).has_value()) {
                continue;
            }

            if (std::filesystem::is_regular_file(path)) {
                AddChild(path.filename(), std::filesystem::file_type::regular);

            } else if (std::filesystem::is_directory(path)) {
                AddChild(path.filename(), std::filesystem::file_type::directory);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TRACER() << "Error: " << e.what();
        return;
    }
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
