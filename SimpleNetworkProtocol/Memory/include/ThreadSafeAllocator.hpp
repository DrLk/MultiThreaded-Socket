#pragma once

#include <cstddef>
#include <memory_resource>
#include <mutex>
#include <unordered_map>

#include "MemoryKeyHash.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Memory {
class ThreadSafeAllocator {
public:
    using AllocationIterator = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>::const_iterator;
    using AllocationNode = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>::node_type;

    AllocationIterator Deallocate(std::pair<AllocationIterator, AllocationIterator> allocations, std::size_t size, std::size_t alignment);
    AllocationNode Allocate(std::size_t size, std::size_t alignment);
    void Deallocate(AllocationNode&& allocation);

private:
    std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash> _pool;

    using Mutex = FastTransport::Thread::SpinLock;
    Mutex _mutex;
};
} // namespace FastTransport::Memory