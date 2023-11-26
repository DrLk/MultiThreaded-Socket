#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include "MemoryKeyHash.hpp"

namespace FastTransport::Memory {

class ThreadSafeAllocator;

class LocalPoolAllocator final {
public:
    static constexpr std::size_t MaxFreeSize = 1024;
    using AllocationIterator = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>::const_iterator;
    using AllocationNode = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>::node_type;

    LocalPoolAllocator(const std::shared_ptr<ThreadSafeAllocator>& globalAllocator, std::pmr::memory_resource* resource);
    LocalPoolAllocator(const LocalPoolAllocator& that) = delete;
    LocalPoolAllocator(LocalPoolAllocator&& that) = delete;
    LocalPoolAllocator& operator=(const LocalPoolAllocator& that) = delete;
    LocalPoolAllocator& operator=(LocalPoolAllocator&& that) = delete;
    ~LocalPoolAllocator();

    void* Allocate(std::size_t size, std::size_t alignment);
    void Deallocate(void* memory, std::size_t size, std::size_t alignment);

private:
    std::pmr::memory_resource* _resource;
    std::shared_ptr<ThreadSafeAllocator> _globalAllocator;
    std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash> _pool;
};

} // namespace FastTransport::Memory