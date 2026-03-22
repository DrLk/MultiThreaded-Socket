#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <fuse3/fuse_lowlevel.h>

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

// RAII handle that unpins a set of Ranges when destroyed.
// Constructed on the cached-tree thread (via Leaf::GetData), destroyed on the
// disk thread when the owning ReadFileCacheJob finishes.
class RangePin {
public:
    RangePin() = default;
    explicit RangePin(std::vector<const Range*> ranges)
        : _ranges(std::move(ranges))
    {
    }
    RangePin(const RangePin&) = delete;
    RangePin& operator=(const RangePin&) = delete;
    RangePin(RangePin&&) = default;
    RangePin& operator=(RangePin&&) = default;
    ~RangePin()
    {
        for (const auto* range : _ranges) {
            range->Unpin();
        }
    }

private:
    std::vector<const Range*> _ranges;
};

// A fuse_bufvec whose buf[i].mem entries point directly into pinned Leaf packet payloads.
// Keeping both together ensures the pin is always held for exactly the lifetime of the buffer.
struct PinnedFuseBufVec {
    PinnedFuseBufVec() = default;
    PinnedFuseBufVec(std::unique_ptr<fuse_bufvec> buf, RangePin rangePin) noexcept
        : bufvec(std::move(buf))
        , pin(std::move(rangePin))
    {
    }

    [[nodiscard]] bool HasData() const noexcept { return bufvec && bufvec->count > 0; }

    // Pointer-like access so callers can use -> and .get() the same way as unique_ptr<fuse_bufvec>.
    fuse_bufvec* operator->() const noexcept { return bufvec.get(); } // NOLINT(fuchsia-overloaded-operator)
    [[nodiscard]] fuse_bufvec* get() const noexcept { return bufvec.get(); }

private:
    std::unique_ptr<fuse_bufvec> bufvec;
    RangePin pin;
} __attribute__((aligned(32)));

} // namespace FastTransport::FileSystem::FileCache
