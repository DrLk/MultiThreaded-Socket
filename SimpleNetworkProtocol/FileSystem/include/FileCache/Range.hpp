#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "IPacket.hpp"
#include "MultiList.hpp"

namespace FastTransport::Protocol {
class IPacket;
} // namespace FastTransport::Protocol

namespace FastTransport::FileSystem::FileCache {

class Range final {
public:
    using Data = Containers::MultiList<std::unique_ptr<Protocol::IPacket>>;
    Range(off_t offset, size_t size, Data&& data);
    Range(const Range&) = delete;
    Range(Range&& other) noexcept;
    Range& operator=(const Range&) = delete;
    Range& operator=(Range&& other) noexcept;

    ~Range();

    void SetOffset(off_t offset);
    [[nodiscard]] off_t GetOffset() const;
    void SetSize(size_t size);
    [[nodiscard]] size_t GetSize() const;

    [[nodiscard]] const Data& GetPackets() const;

    [[nodiscard]] Data& GetPackets();

    void AddRef();
    void Release();

    // Pin/Unpin keep the Range alive in the Leaf cache while an async job holds a
    // fuse_bufvec that points directly into this Range's packet payloads.
    // Pin() is called from the cached-tree thread; Unpin() may be called from any thread.
    void Pin() const noexcept { _counter.fetch_add(1, std::memory_order_relaxed); }
    void Unpin() const noexcept { _counter.fetch_sub(1, std::memory_order_relaxed); }
    [[nodiscard]] bool IsPinned() const noexcept { return _counter.load(std::memory_order_relaxed) > 1; }

    bool operator<(const Range& other) const; // NOLINT(fuchsia-overloaded-operator)

private:
    off_t _offset;
    size_t _size;
    Data _data;
    mutable std::atomic<int> _counter { 1 };
};

// Transparent comparator used by std::set<std::shared_ptr<Range>, ...> so the
// owning containers (e.g. Leaf::_data) can keep ranges alive via shared_ptr
// without sacrificing the natural Range ordering.
struct RangePtrLess {
    using is_transparent = void;
    bool operator()(const std::shared_ptr<Range>& lhs, const std::shared_ptr<Range>& rhs) const { return *lhs < *rhs; } // NOLINT(fuchsia-overloaded-operator)
    bool operator()(const std::shared_ptr<Range>& lhs, const Range& rhs) const { return *lhs < rhs; } // NOLINT(fuchsia-overloaded-operator)
    bool operator()(const Range& lhs, const std::shared_ptr<Range>& rhs) const { return lhs < *rhs; } // NOLINT(fuchsia-overloaded-operator)
};

// RAII handle that unpins a set of Ranges when destroyed.
// Constructed on the cached-tree thread (via Leaf::GetData), destroyed on the
// disk thread when the owning ReadFileCacheJob finishes. Owns shared_ptr to
// keep the Range alive even if Leaf::ExtractBlock evicts the cache entry while
// the disk thread still holds the pin.
class RangePin {
public:
    RangePin() = default;
    explicit RangePin(std::vector<std::shared_ptr<const Range>> ranges)
        : _ranges(std::move(ranges))
    {
    }
    RangePin(const RangePin&) = delete;
    RangePin& operator=(const RangePin&) = delete;
    RangePin(RangePin&&) = default;
    RangePin& operator=(RangePin&&) = default;
    ~RangePin()
    {
        for (const auto& range : _ranges) {
            range->Unpin();
        }
    }

private:
    std::vector<std::shared_ptr<const Range>> _ranges;
};

} // namespace FastTransport::FileSystem::FileCache
