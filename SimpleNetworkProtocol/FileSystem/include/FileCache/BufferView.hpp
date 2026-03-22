#pragma once

#include <cstddef>
#include <vector>

#include "FileCache/Range.hpp"

namespace FastTransport::FileSystem::FileCache {

struct Span {
    const std::byte* data;
    size_t size;
};

struct BufferView {
    std::vector<Span> spans;
    RangePin pin;

    [[nodiscard]] bool HasData() const noexcept { return !spans.empty(); }
    [[nodiscard]] size_t TotalSize() const noexcept
    {
        size_t total = 0;
        for (const auto& span : spans) {
            total += span.size;
        }
        return total;
    }
};

} // namespace FastTransport::FileSystem::FileCache
