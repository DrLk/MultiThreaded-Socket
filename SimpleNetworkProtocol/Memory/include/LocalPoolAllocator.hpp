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
    using AllocationPool = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>;
    using AllocationIterator = AllocationPool::const_iterator;
    using AllocationNode = AllocationPool::node_type;

    explicit LocalPoolAllocator(const std::shared_ptr<ThreadSafeAllocator>& globalAllocator);
    LocalPoolAllocator(const LocalPoolAllocator& that) = delete;
    LocalPoolAllocator(LocalPoolAllocator&& that) = delete;
    LocalPoolAllocator& operator=(const LocalPoolAllocator& that) = delete;
    LocalPoolAllocator& operator=(LocalPoolAllocator&& that) = delete;
    ~LocalPoolAllocator();

    void* Allocate(std::size_t size, std::size_t alignment);
    void Deallocate(void* memory, std::size_t size, std::size_t alignment);

private:
    std::shared_ptr<ThreadSafeAllocator> _globalAllocator;
    AllocationPool _pool;
    std::unordered_map<std::pair<std::size_t, std::size_t>, std::size_t, MemoryKeyHash> _backetSize;
};

} // namespace FastTransport::Memory