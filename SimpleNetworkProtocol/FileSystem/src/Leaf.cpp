#include "Leaf.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include "IPacket.hpp"
#include "Logger.hpp"

#define TRACER() LOGGER() << "[Leaf] " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::FileSystem {

Leaf::Leaf(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size, Leaf* parent)
    : _name(std::move(name))
    , _type(type)
    , _size(size)
    , _parent(parent)
    , _piecesStatus(std::make_shared<PiecesStatus>((size / BlockSize) + 1))
{
}

Leaf::Leaf(Leaf&& that) noexcept = default;

Leaf& Leaf::operator=(Leaf&& that) noexcept = default;

Leaf::~Leaf() = default;

Leaf& Leaf::AddChild(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size)
{
    Leaf leaf(std::move(name), type, size, this);
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
    return _size;
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
                const uintmax_t size = std::filesystem::file_size(path);
                AddChild(path.filename(), std::filesystem::file_type::regular, size);

            } else if (std::filesystem::is_directory(path)) {
                AddChild(path.filename(), std::filesystem::file_type::directory, 0);
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

std::filesystem::path Leaf::GetCachePath() const
{
    std::filesystem::path path = GetName();
    return path;
}

Leaf::Data Leaf::AddData(off_t offset, size_t size, Data&& data)
{
    auto block = _data.find(offset / BlockSize);
    if (block == _data.end()) {
        std::set<FileCache::Range> blocks;
        blocks.insert(FileCache::Range(offset, size, std::move(data)));
        _data.insert({ offset / BlockSize, std::move(blocks) });
        _piecesStatus->SetStatus(offset / BlockSize, PieceStatus::InCache);
        return {};
    }

    std::set<FileCache::Range>& blocks = block->second;
    auto oldData = blocks.upper_bound(FileCache::Range(offset, size, Data {}));

    if (oldData != blocks.end() && oldData->GetOffset() == offset + size) {
        if (oldData != blocks.begin()) {
            auto node = blocks.extract(oldData);
            data.splice(std::move(node.value().GetPackets()));
            auto prevData = oldData;
            --prevData;
            if (prevData->GetOffset() + prevData->GetSize() == offset) {
                auto prevNode = blocks.extract(prevData);
                prevNode.value().GetPackets().splice(std::move(data));
                prevNode.value().SetSize(prevNode.value().GetSize() + size + node.value().GetSize());
                blocks.insert(std::move(prevNode));
                return {};
            }
        }
        auto node = blocks.extract(oldData);
        data.splice(std::move(node.value().GetPackets()));
        node.value().GetPackets() = std::move(data);
        node.value().SetSize(node.value().GetSize() + size);
        node.value().SetOffset(offset);
        blocks.insert(std::move(node));
        return {};
    }

    if (oldData == blocks.begin()) {
        blocks.insert(FileCache::Range(offset, size, std::move(data)));
        if (oldData->GetOffset() >= offset && oldData->GetOffset() + oldData->GetSize() <= offset + size) {
            auto removedData = blocks.extract(oldData);
            return std::move(removedData.value().GetPackets());
        }
        return {};
    }

    --oldData;
    if (oldData->GetOffset() <= offset && offset <= oldData->GetOffset() + oldData->GetSize()) {
        auto node = blocks.extract(oldData);

        if (node.value().GetOffset() + node.value().GetSize() == offset) {
            node.value().GetPackets().splice(std::move(data));
            node.value().SetSize(node.value().GetSize() + size);
            blocks.insert(std::move(node));
            return {};
        }

        assert(node.value().GetOffset() + node.value().GetSize() > offset);

        const size_t skipSize = node.value().GetOffset() + node.value().GetSize() - offset;
        if (node.value().GetPackets().size() > 1) {
            const size_t blockSize = node.value().GetPackets().front()->GetPayload().size();
            auto skipData = data.TryGenerate((skipSize + blockSize - 1) / blockSize);

            node.value().GetPackets().splice(std::move(data));
            node.value().SetSize(node.value().GetSize() + size - skipSize);
            blocks.insert(std::move(node));
            return skipData;
        }
    }

    blocks.insert(FileCache::Range(offset, size, std::move(data)));
    return {};
}

std::pair<off_t, Leaf::Data> Leaf::ExtractBlock(size_t index)
{
    Leaf::Data extractedData;
    auto block = _data.find(index);

    if (block == _data.end()) {
        return { 0, {} };
    }

    _piecesStatus->SetStatus(index, PieceStatus::NotFound);

    std::set<FileCache::Range>& ranges = block->second;

    const off_t offset = ranges.begin()->GetOffset();
    for (auto it = ranges.begin(); it != ranges.end();) {
        auto extractedIt = it;
        it++;
        auto range = ranges.extract(extractedIt);
        extractedData.splice(std::move(range.value().GetPackets()));
    }

    _data.erase(block);
    return { offset, std::move(extractedData) };
}

size_t Leaf::GetFirstBlockIndex() const
{
    if (_data.empty()) {
        return SIZE_MAX;
    }
    size_t minIndex = SIZE_MAX;
    for (const auto& [index, ranges] : _data) {
        const bool anyPinned = std::ranges::any_of(ranges,
            [](const FileCache::Range& range) { return range.IsPinned(); });
        if (!anyPinned) {
            minIndex = std::min(minIndex, index);
        }
    }
    return minIndex;
}

FileCache::BufferView Leaf::GetData(off_t offset, size_t size) const
{
    std::vector<FileCache::Span> spans;
    std::vector<const FileCache::Range*> pinnedRanges;

    if (std::cmp_greater_equal(offset, _size)) {
        return {};
    }
    size_t readed = std::min(size, _size - static_cast<size_t>(offset));

    while (readed > 0) {
        auto block = _data.find(offset / BlockSize);
        if (block == _data.end()) {
            return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
        }

        const std::set<FileCache::Range>& blocks = block->second;
        auto data = blocks.upper_bound(FileCache::Range(offset, size, Data {}));
        if (data == blocks.begin()) {
            return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
        }

        --data;

        if (offset >= data->GetOffset() && offset <= data->GetOffset() + data->GetSize()) {
            // Pin this range so GetFirstBlockIndex skips it until the job is done.
            data->Pin();
            pinnedRanges.push_back(&*data);

            const auto& packets = data->GetPackets();

            if (packets.empty()) {
                throw std::runtime_error("Data not found");
            }

            size_t start = offset - data->GetOffset();

            auto packet = packets.begin();
            while (start >= (*packet)->GetPayload().size()) {
                start -= (*packet)->GetPayload().size();
                packet++;
                if (packet == packets.end()) {
                    return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
                }
            }

            const auto firstPayload = (*packet)->GetPayload().subspan(start);
            const size_t firstSize = std::min(firstPayload.size(), readed);
            spans.push_back({ .data = reinterpret_cast<const std::byte*>(firstPayload.data()), .size = firstSize }); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            readed -= firstSize;
            offset += static_cast<off_t>(firstSize);
            packet++;

            for (; packet != packets.end() && readed != 0; packet++) {
                const auto payload = (*packet)->GetPayload();
                const size_t spanSize = std::min(payload.size(), readed);
                spans.push_back({ .data = reinterpret_cast<const std::byte*>(payload.data()), .size = spanSize }); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                readed -= spanSize;
                offset += static_cast<off_t>(spanSize);
            }
        } else {
            return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
        }
    }

    return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
}

std::shared_ptr<PiecesStatus> Leaf::GetPiecesStatus()
{
    return _piecesStatus;
}

bool Leaf::SetInFlight(size_t blockIndex)
{
    if (_piecesStatus->GetStatus(blockIndex) == PieceStatus::NotFound) {
        _piecesStatus->SetStatus(blockIndex, PieceStatus::InFlight);
        return true;
    }
    return false;
}

void Leaf::AddPendingJob(size_t blockIndex, std::unique_ptr<IPendingJob> job)
{
    _pendingJobs[blockIndex].push_back(std::move(job));
}

std::vector<std::unique_ptr<IPendingJob>> Leaf::TakePendingJobs(size_t blockIndex)
{
    auto iter = _pendingJobs.find(blockIndex);
    if (iter == _pendingJobs.end()) {
        return {};
    }
    auto result = std::move(iter->second);
    _pendingJobs.erase(iter);
    return result;
}

void Leaf::CancelAllPendingJobs()
{
    // Iterative breadth-first traversal to avoid recursion (misc-no-recursion).
    std::vector<Leaf*> queue = { this };
    while (!queue.empty()) {
        Leaf* current = queue.back();
        queue.pop_back();
        for (auto& [blockIndex, jobs] : current->_pendingJobs) {
            TRACER() << "Cancelling " << jobs.size() << " pending jobs for block index " << blockIndex << " of file " << current->GetFullPath();
            for (auto& job : jobs) {
                job->Cancel();
            }
        }
        current->_pendingJobs.clear();
        for (auto& [name, child] : current->children) {
            queue.push_back(&child);
        }
    }
}

} // namespace FastTransport::FileSystem
