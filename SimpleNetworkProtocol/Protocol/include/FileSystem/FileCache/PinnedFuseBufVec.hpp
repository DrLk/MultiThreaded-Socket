#pragma once

#include <bit>
#include <cstddef>
#include <memory>

#include <fuse3/fuse_lowlevel.h>

#include "FileCache/BufferView.hpp"
#include "FileCache/Range.hpp"

namespace FastTransport::FileSystem::FileCache {

// A fuse_bufvec whose buf[i].mem entries point directly into pinned Leaf packet payloads.
// Keeping both together ensures the pin is always held for exactly the lifetime of the buffer.
class PinnedFuseBufVec {
public:
    PinnedFuseBufVec() = default;
    PinnedFuseBufVec(std::unique_ptr<fuse_bufvec> buf, RangePin rangePin) noexcept
        : _bufvec(std::move(buf))
        , _pin(std::move(rangePin))
    {
    }

    [[nodiscard]] bool HasData() const noexcept { return _bufvec && _bufvec->count > 0; }

    fuse_bufvec* operator->() const noexcept { return _bufvec.get(); } // NOLINT(fuchsia-overloaded-operator)
    [[nodiscard]] fuse_bufvec* get() const noexcept { return _bufvec.get(); }

private:
    std::unique_ptr<fuse_bufvec> _bufvec;
    RangePin _pin;
};

// Build a PinnedFuseBufVec from a BufferView: each span becomes a memory fuse_buf entry.
// The RangePin is transferred so the backing Ranges stay pinned for the lifetime of the result.
[[nodiscard]] inline PinnedFuseBufVec buildPinnedBufVec(BufferView view)
{
    const std::size_t count = view.spans.size();
    if (count == 0) {
        return {};
    }
    const std::size_t allocSize = sizeof(fuse_bufvec) + ((count - 1) * sizeof(fuse_buf));
    std::unique_ptr<fuse_bufvec> fvec(reinterpret_cast<fuse_bufvec*>(new char[allocSize])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    fvec->count = count;
    fvec->idx = 0;
    fvec->off = 0;
    for (std::size_t i = 0; i < count; ++i) {
        fvec->buf[i] = {}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        fvec->buf[i].mem = const_cast<void*>(static_cast<const void*>(view.spans[i].data)); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-type-const-cast)
        fvec->buf[i].size = view.spans[i].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        fvec->buf[i].flags = std::bit_cast<fuse_buf_flags>(0); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    return PinnedFuseBufVec { std::move(fvec), std::move(view.pin) };
}

} // namespace FastTransport::FileSystem::FileCache
