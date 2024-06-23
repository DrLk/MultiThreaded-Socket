#include "Leaf.hpp"

#include <cstdint>
#include <filesystem>
#include <fuse3/fuse_lowlevel.h>
#include <stdexcept>
#include <string>

#include "IPacket.hpp"
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

Leaf::Data Leaf::AddData(off_t offset, size_t size, Data&& data)
{
    auto oldData = _data.upper_bound(FileCache::Range(offset, size, Data {}));
    if (oldData == _data.begin()) {
        _data.insert(FileCache::Range(offset, size, std::move(data)));
        return {};
    }

    --oldData;
    auto node = _data.extract(oldData);
    if (node.value().GetOffset() + node.value().GetSize() == offset) {
        node.value().GetPackets().splice(std::move(data));
        node.value().SetSize(node.value().GetSize() + size);
        _data.insert(std::move(node));
        return {};
    }

    return {};
    assert(false);
    node.value().GetPackets().splice(std::move(data));
    node.value().SetSize(node.value().GetSize() + size);
    _data.insert(std::move(node));

    return {};
}

std::unique_ptr<fuse_bufvec> Leaf::GetData(off_t offset, size_t size) const
{
    auto data = _data.upper_bound(FileCache::Range(offset, size, Data {}));
    if (data == _data.begin()) {
        return nullptr;
    }

    --data; // Move to the previous range

    if (offset >= data->GetOffset() && offset <= data->GetOffset() + data->GetSize()) {

        const auto& packets = data->GetPackets();

        if (packets.empty()) {
            throw std::runtime_error("Data not found");
        }

        size_t start = offset - data->GetOffset();

        auto packet = packets.begin();
        while (start >= (*packet)->GetPayload().size()) {
            start -= (*packet)->GetPayload().size();
            packet++;
        }

        size_t readed = size;
        const std::size_t length = sizeof(fuse_bufvec) + sizeof(fuse_buf) * (size / 1400);
        std::unique_ptr<fuse_bufvec> buffVector(reinterpret_cast<fuse_bufvec*>(new char[length])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        buffVector->count = 1;
        buffVector->off = 0;
        buffVector->idx = 0;

        auto& buffer = buffVector->buf[0]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer.mem = (*packet)->GetPayload().data() + start;
        buffer.size = std::min((*packet)->GetPayload().size() - start, readed);
        buffer.flags = static_cast<fuse_buf_flags>(0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        buffer.pos = 0;
        buffer.fd = 0;

        readed -= buffer.size;
        int index = 1;
        for (packet++; packet != packets.end() && readed != 0; packet++) {
            auto& buffer = buffVector->buf[index++]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            buffer.mem = (*packet)->GetPayload().data();
            buffer.size = std::min((*packet)->GetPayload().size(), readed);
            buffer.flags = static_cast<fuse_buf_flags>(0); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
            buffer.pos = 0;
            buffer.fd = 0;

            readed -= buffer.size;
            buffVector->count++;
        }

        return buffVector;
    }

    throw std::runtime_error("Data not found");
}

} // namespace FastTransport::FileSystem
