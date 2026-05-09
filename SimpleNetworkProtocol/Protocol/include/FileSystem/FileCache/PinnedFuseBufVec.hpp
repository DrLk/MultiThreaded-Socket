#pragma once

#include <bit>
#include <cstddef>
#include <memory>

#include <fuse3/fuse_lowlevel.h>

#include "FileCache/BufferView.hpp"
#include "FileCache/Range.hpp"

namespace FastTransport::FileSystem::FileCache {

// fuse_bufvec is allocated as a flexible array via `new char[]`, so it must be
// deallocated as `char[]` to keep ASan's new[]/delete[] pairing happy.
struct FuseBufVecDeleter {
    void operator()(fuse_bufvec* ptr) const noexcept // NOLINT(fuchsia-overloaded-operator)
    {
        delete[] reinterpret_cast<char*>(ptr); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-owning-memory)
    }
};

using FuseBufVecPtr = std::unique_ptr<fuse_bufvec, FuseBufVecDeleter>;

// fuse_bufvec is a flexible-array struct (the buf[1] member is really buf[count]),
// so the entire object must be allocated as a raw byte buffer of the right size and
// reinterpret-cast back. This helper centralises the cast so callers don't each
// need their own NOLINT comment.
[[nodiscard]] inline FuseBufVecPtr AllocateFuseBufVec(std::size_t count)
{
    const std::size_t allocSize = sizeof(fuse_bufvec) + ((count > 0 ? count - 1 : 0) * sizeof(fuse_buf));
    return FuseBufVecPtr(reinterpret_cast<fuse_bufvec*>(new char[allocSize])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

// A fuse_bufvec whose buf[i].mem entries point directly into pinned Leaf packet payloads.
// Keeping both together ensures the pin is always held for exactly the lifetime of the buffer.
class PinnedFuseBufVec {
public:
    PinnedFuseBufVec() = default;
    PinnedFuseBufVec(FuseBufVecPtr buf, RangePin rangePin) noexcept
        : _bufvec(std::move(buf))
        , _pin(std::move(rangePin))
    {
    }

    [[nodiscard]] bool HasData() const noexcept { return _bufvec && _bufvec->count > 0; }

    fuse_bufvec* operator->() const noexcept { return _bufvec.get(); } // NOLINT(fuchsia-overloaded-operator)
    [[nodiscard]] fuse_bufvec* get() const noexcept { return _bufvec.get(); }

private:
    FuseBufVecPtr _bufvec;
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
    auto fvec = AllocateFuseBufVec(count);
    fvec->count = count;
    fvec->idx = 0;
    fvec->off = 0;
    for (std::size_t i = 0; i < count; ++i) {
        fvec->buf[i] = {}; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index, bugprone-invalid-enum-default-initialization)
        fvec->buf[i].mem = const_cast<void*>(static_cast<const void*>(view.spans[i].data)); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-type-const-cast)
        fvec->buf[i].size = view.spans[i].size; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        fvec->buf[i].flags = std::bit_cast<fuse_buf_flags>(0); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    return PinnedFuseBufVec { std::move(fvec), std::move(view.pin) };
}

} // namespace FastTransport::FileSystem::FileCache
