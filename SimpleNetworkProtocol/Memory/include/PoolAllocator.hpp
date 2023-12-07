#pragma once

#include <cstddef>
#include <memory>
#include <memory_resource>

#include "LocalPoolAllocator.hpp"
#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {
class PoolAllocator : public std::pmr::memory_resource {
public:
    explicit PoolAllocator(std::pmr::memory_resource* resource);

private:
    [[nodiscard]] void* do_allocate(std::size_t size, std::size_t alignment) override;
    void do_deallocate(void* memory, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    LocalPoolAllocator& GetThreadPool();

    std::shared_ptr<ThreadSafeAllocator> _globalAllocator;
};

} // namespace FastTransport::Memory