#pragma once

#include <cstddef>
#include <memory>
#include <memory_resource>

#include "LocalPoolAllocator.hpp"
#include "MemoryLogger.hpp"
#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {
class PoolAllocator : public std::pmr::memory_resource {
public:
    explicit PoolAllocator();

private:
    static MemoryLogger MemoryResource; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    static std::shared_ptr<ThreadSafeAllocator> GlobalAllocator; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    static thread_local LocalPoolAllocator _threadResources; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    [[nodiscard]] void* do_allocate(std::size_t size, std::size_t alignment) override;
    void do_deallocate(void* memory, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    std::pmr::memory_resource* _resource;
};

} // namespace FastTransport::Memory