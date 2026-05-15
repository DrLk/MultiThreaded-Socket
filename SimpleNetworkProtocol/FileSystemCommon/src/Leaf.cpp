#include "Leaf.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "FileTree.hpp"
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

Leaf::~Leaf() = default;

Leaf& Leaf::AddChild(std::filesystem::path&& name, std::filesystem::file_type type, uintmax_t size)
{
    auto leaf = std::make_shared<Leaf>(std::move(name), type, size, this);
    leaf->_tree = _tree;
    auto [insertedLeaf, result] = children.insert({ leaf->GetName().native(), leaf });
    auto& childPtr = insertedLeaf->second;
    Leaf& child = *childPtr;
    if (_tree != nullptr && result) {
        _tree->RegisterLeaf(child.GetServerInode(), childPtr);
    }
    return child;
}

Leaf& Leaf::AddChild(std::shared_ptr<Leaf> leaf)
{
    auto [insertedLeaf, result] = children.insert({ leaf->GetName().native(), std::move(leaf) });
    auto& rootPtr = insertedLeaf->second;
    Leaf& child = *rootPtr;
    if (_tree != nullptr && result) {
        // Walk the freshly-spliced subtree, fix up _tree, and register every node.
        std::vector<std::shared_ptr<Leaf>> queue { rootPtr };
        while (!queue.empty()) {
            auto current = std::move(queue.back());
            queue.pop_back();
            current->_tree = _tree;
            _tree->RegisterLeaf(current->GetServerInode(), current);
            for (auto& [childName, grandchild] : current->children) {
                queue.push_back(grandchild);
            }
        }
    } else {
        child._tree = _tree;
    }
    return child;
}

void Leaf::RemoveChild(const std::string& name)
{
    auto entry = children.find(name);
    if (entry == children.end()) {
        return;
    }
    if (_tree != nullptr) {
        // Unregister the entire subtree before erasing so no stale pointers
        // remain in the index.
        std::vector<Leaf*> queue { entry->second.get() };
        while (!queue.empty()) {
            Leaf* current = queue.back();
            queue.pop_back();
            _tree->UnregisterLeaf(current->GetServerInode());
            for (auto& [childName, grandchild] : current->children) {
                queue.push_back(grandchild.get());
            }
        }
    }
    // Drop the parent's shared_ptr. If the kernel still holds nlookup
    // references (_selfPin set), the Leaf survives until the last ReleaseRef
    // (routed via FuseForget → ReleaseLeafRefJob) releases the pin. Otherwise
    // the Leaf is destroyed here.
    children.erase(entry);
}

void Leaf::SetServerInode(std::uint64_t inode)
{
    if (_tree != nullptr) {
        _tree->UnregisterLeaf(GetServerInode());
    }
    _serverInode = inode;
    if (_tree != nullptr) {
        _tree->RegisterLeaf(GetServerInode(), shared_from_this());
    }
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
    // Single-thread invariant: only called from _mainQueue worker. The very
    // first ref captures _selfPin so the Leaf survives any subsequent
    // RemoveChild that drops the parent's shared_ptr.
    if (_nlookup.fetch_add(1, std::memory_order_relaxed) == 0) {
        _selfPin = shared_from_this();
    }
}

void Leaf::ReleaseRef() const
{
    Leaf::ReleaseRef(1);
}

void Leaf::DropPinsRecursively() noexcept
{
    // Iterative BFS to avoid recursion (misc-no-recursion).
    std::vector<Leaf*> queue { this };
    while (!queue.empty()) {
        Leaf* current = queue.back();
        queue.pop_back();
        const std::uint64_t nlookup = current->_nlookup.load(std::memory_order_relaxed);
        if (nlookup != 0 && current->_selfPin != nullptr) {
            // Pin-balance violation: kernel still believes it holds references
            // but we're tearing the tree down. Indicates lost forget delivery.
            try {
                TRACER() << "DropPinsRecursively: " << current->GetFullPath()
                         << " still has nlookup=" << nlookup << " at shutdown";
            } catch (...) { // NOLINT(bugprone-empty-catch)
                // Logging is best-effort here: DropPinsRecursively is noexcept
                // and runs during shutdown, so a throwing logger must not
                // propagate or terminate the process.
            }
        }
        current->_selfPin.reset();
        for (auto& [name, child] : current->children) {
            queue.push_back(child.get());
        }
    }
}

void Leaf::ReleaseRef(uint64_t nlookup) const
{
    // Single-thread invariant: only called from _mainQueue worker. Resetting
    // _selfPin may run ~Leaf if it was the last shared_ptr — `released` is
    // moved out of the field first so the destructor runs after the local
    // shared_ptr falls out of scope, with no further `this->` access.
    const uint64_t prev = _nlookup.fetch_sub(nlookup, std::memory_order_relaxed);
    assert(prev >= nlookup);
    if (prev == nlookup) {
        const std::shared_ptr<const Leaf> released = std::move(_selfPin);
    }
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

const std::map<std::string, std::shared_ptr<Leaf>>& Leaf::GetChildren() const
{
    return children;
}

std::optional<std::reference_wrapper<const Leaf>> Leaf::Find(const std::string& name) const
{
    auto file = children.find(name);
    if (file != children.end()) {
        return std::cref(*file->second);
    }

    return {};
}

std::shared_ptr<Leaf> Leaf::FindChild(const std::string& name)
{
    auto file = children.find(name);
    if (file == children.end()) {
        return nullptr;
    }
    return file->second;
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
        std::set<std::shared_ptr<FileCache::Range>, FileCache::RangePtrLess> blocks;
        blocks.insert(std::make_shared<FileCache::Range>(offset, size, std::move(data)));
        _data.insert({ offset / BlockSize, std::move(blocks) });
        _piecesStatus->SetStatus(offset / BlockSize, PieceStatus::InCache);
        return {};
    }

    auto& blocks = block->second;
    auto oldData = blocks.upper_bound(FileCache::Range(offset, size, Data {}));

    if (oldData != blocks.end() && (*oldData)->GetOffset() == offset + size) {
        if (oldData != blocks.begin()) {
            auto& nextRange = **oldData;
            data.splice(std::move(nextRange.GetPackets()));
            auto prevIt = oldData;
            --prevIt;
            if ((*prevIt)->GetOffset() + (*prevIt)->GetSize() == offset) {
                auto& prevRange = **prevIt;
                prevRange.GetPackets().splice(std::move(data));
                prevRange.SetSize(prevRange.GetSize() + size + nextRange.GetSize());
                blocks.erase(oldData);
                return {};
            }
        }
        auto& nextRange = **oldData;
        data.splice(std::move(nextRange.GetPackets()));
        nextRange.GetPackets() = std::move(data);
        nextRange.SetSize(nextRange.GetSize() + size);
        nextRange.SetOffset(offset);
        return {};
    }

    if (oldData == blocks.begin()) {
        blocks.insert(std::make_shared<FileCache::Range>(offset, size, std::move(data)));
        if ((*oldData)->GetOffset() >= offset && (*oldData)->GetOffset() + (*oldData)->GetSize() <= offset + size) {
            auto removedNode = blocks.extract(oldData);
            return std::move(removedNode.value()->GetPackets());
        }
        return {};
    }

    --oldData;
    if ((*oldData)->GetOffset() <= offset && offset <= (*oldData)->GetOffset() + (*oldData)->GetSize()) {
        auto& prevRange = **oldData;

        if (prevRange.GetOffset() + prevRange.GetSize() == offset) {
            prevRange.GetPackets().splice(std::move(data));
            prevRange.SetSize(prevRange.GetSize() + size);
            return {};
        }

        assert(prevRange.GetOffset() + prevRange.GetSize() > offset);

        const size_t skipSize = prevRange.GetOffset() + prevRange.GetSize() - offset;
        if (prevRange.GetPackets().size() > 1) {
            const size_t blockSize = prevRange.GetPackets().front()->GetPayload().size();
            auto skipData = data.TryGenerate((skipSize + blockSize - 1) / blockSize);

            prevRange.GetPackets().splice(std::move(data));
            prevRange.SetSize(prevRange.GetSize() + size - skipSize);
            return skipData;
        }
    }

    blocks.insert(std::make_shared<FileCache::Range>(offset, size, std::move(data)));
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

    auto& ranges = block->second;

    const off_t offset = (*ranges.begin())->GetOffset();
    for (const auto& range : ranges) {
        extractedData.splice(std::move(range->GetPackets()));
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
            [](const std::shared_ptr<FileCache::Range>& range) { return range->IsPinned(); });
        if (!anyPinned) {
            minIndex = std::min(minIndex, index);
        }
    }
    return minIndex;
}

FileCache::BufferView Leaf::GetData(off_t offset, size_t size) const
{
    std::vector<FileCache::Span> spans;
    std::vector<std::shared_ptr<const FileCache::Range>> pinnedRanges;

    if (std::cmp_greater_equal(offset, _size)) {
        return {};
    }
    size_t readed = std::min(size, _size - static_cast<size_t>(offset));

    while (readed > 0) {
        auto block = _data.find(offset / BlockSize);
        if (block == _data.end()) {
            return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
        }

        const auto& blocks = block->second;
        auto dataIt = blocks.upper_bound(FileCache::Range(offset, size, Data {}));
        if (dataIt == blocks.begin()) {
            return { .spans = std::move(spans), .pin = FileCache::RangePin(std::move(pinnedRanges)) };
        }

        --dataIt;

        const auto& dataRange = **dataIt;
        if (offset >= dataRange.GetOffset() && offset <= dataRange.GetOffset() + dataRange.GetSize()) {
            // Pin this range so GetFirstBlockIndex skips it until the job is done.
            dataRange.Pin();
            pinnedRanges.push_back(*dataIt);

            const auto& packets = dataRange.GetPackets();

            if (packets.empty()) {
                throw std::runtime_error("Data not found");
            }

            size_t start = offset - dataRange.GetOffset();

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

void Leaf::InvalidateDataCache()
{
    _data.clear();
    _piecesStatus = std::make_shared<PiecesStatus>((_size / BlockSize) + 1);
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
            queue.push_back(child.get());
        }
    }
}

} // namespace FastTransport::FileSystem
