#pragma once

#include <cstddef>
#include <memory_resource>

namespace FastTransport::Memory {

class MemoryLogger : public std::pmr::memory_resource {
public:
    explicit MemoryLogger(std::pmr::memory_resource* resource);

    [[nodiscard]] std::size_t GetAllocatedSize() const;

private:
    [[nodiscard]] void* do_allocate(std::size_t size, std::size_t alignment) override;
    void do_deallocate(void* memory, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    std::pmr::memory_resource* _resource;
    std::size_t _allocatedSize;
};
} // namespace FastTransport::Memory