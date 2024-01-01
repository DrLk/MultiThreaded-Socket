#include "ThreadSafeAllocator.hpp"

#include <mutex>

namespace FastTransport::Memory {

ThreadSafeAllocator::ThreadSafeAllocator(std::pmr::memory_resource* resource)
    : _resource(resource)
{
}

ThreadSafeAllocator::~ThreadSafeAllocator() = default;

void* ThreadSafeAllocator::Allocate(std::size_t size, std::size_t alignment)
{
    return _resource->allocate(size, alignment);
}

ThreadSafeAllocator::AllocationNodes ThreadSafeAllocator::Allocate2(std::size_t size, std::size_t alignment)
{
    AllocationNodes nodes;
    {
        const std::scoped_lock<Mutex> lock(_mutex);
        for (auto& node : nodes) {
            node = _pool.extract({ size, alignment });
            if (node.empty()) {
                break;
            }
        }
    }

    return nodes;
}

void ThreadSafeAllocator::Deallocate(AllocationNode&& allocation)
{
    if (allocation.empty()) {
        return;
    }

    const std::scoped_lock<Mutex> lock(_mutex);
    _pool.insert(std::move(allocation));
}

void ThreadSafeAllocator::Deallocate(AllocationPool&& pool)
{
    if (pool.empty()) {
        return;
    }

    const std::scoped_lock<Mutex> lock(_mutex);
    _pool.merge(std::move(pool));
}

void ThreadSafeAllocator::Deallocate(AllocationNodes&& nodes) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    for (auto&& node : nodes) {
        if (node.empty()) {
            break;
        }

        _pool.insert(std::move(node));
    }
}
} // namespace FastTransport::Memory
