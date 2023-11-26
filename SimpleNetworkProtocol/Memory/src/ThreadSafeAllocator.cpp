#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {

ThreadSafeAllocator::AllocationIterator ThreadSafeAllocator::Deallocate(std::pair<AllocationIterator, AllocationIterator> allocations, std::size_t size, std::size_t alignment)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    auto [begin, end] = allocations;
    auto iterator = begin;
    for (; iterator != end; iterator++) {
        _pool.insert(*iterator);
    }

    return iterator;
}

ThreadSafeAllocator::AllocationNode ThreadSafeAllocator::Allocate(std::size_t size, std::size_t alignment)
{
    const std::scoped_lock<Mutex> lock(_mutex);

    return _pool.extract({ size, alignment });
}

void ThreadSafeAllocator::Deallocate(AllocationNode&& allocation)
{
    const std::scoped_lock<Mutex> lock(_mutex);
    _pool.insert(std::move(allocation));
}
} // namespace FastTransport::Memory