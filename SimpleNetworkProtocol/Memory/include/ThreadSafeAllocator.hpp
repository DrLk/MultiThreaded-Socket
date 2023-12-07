#pragma once

#include <array>
#include <cstddef>
#include <memory_resource>
#include <unordered_map>

#include "MemoryKeyHash.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Memory {
class ThreadSafeAllocator final {
public:
    static constexpr std::size_t MaxFreeSize = 1024;

    using AllocationPool = std::unordered_multimap<std::pair<std::size_t, std::size_t>, void*, MemoryKeyHash>;
    using AllocationNode = AllocationPool::node_type;
    using AllocationNodes = std::array<AllocationNode, MaxFreeSize>;

    explicit ThreadSafeAllocator(std::pmr::memory_resource* resource);
    ThreadSafeAllocator(const ThreadSafeAllocator& that) = delete;
    ThreadSafeAllocator(ThreadSafeAllocator&& that) = delete;
    ThreadSafeAllocator& operator=(const ThreadSafeAllocator& that) = delete;
    ThreadSafeAllocator& operator=(ThreadSafeAllocator&& that) = delete;
    ~ThreadSafeAllocator();

    void* Allocate(std::size_t size, std::size_t alignment);
    AllocationNodes Allocate2(std::size_t size, std::size_t alignment);
    void Deallocate(AllocationNode&& allocation);
    void Deallocate(AllocationPool&& pool);
    void Deallocate(AllocationNodes&& nodes);

private:
    AllocationPool _pool;

    using Mutex = FastTransport::Thread::SpinLock;
    Mutex _mutex;

    std::pmr::memory_resource* _resource;
};
} // namespace FastTransport::Memory