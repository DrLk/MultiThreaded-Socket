#pragma once

#include <cstddef>
#include <vector>

#include "FileCache/Range.hpp"

namespace FastTransport::FileSystem::FileCache {

struct Span {
    const std::byte* data;
    size_t size;
} __attribute__((aligned(16)));

struct BufferView {
    std::vector<Span> spans; // NOLINT(misc-non-private-member-variables-in-classes)
    RangePin pin; // NOLINT(misc-non-private-member-variables-in-classes)

    [[nodiscard]] bool HasData() const noexcept { return !spans.empty(); }
    [[nodiscard]] size_t TotalSize() const noexcept
    {
        size_t total = 0;
        for (const auto& span : spans) {
            total += span.size;
        }
        return total;
    }
} __attribute__((aligned(64)));

} // namespace FastTransport::FileSystem::FileCache
