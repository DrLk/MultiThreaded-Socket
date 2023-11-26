#include "PoolAllocator.hpp"

#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {

PoolAllocator::PoolAllocator()
{
}

void* PoolAllocator::do_allocate(std::size_t size, std::size_t alignment)
{
    return _threadResources.Allocate(size, alignment);
}

void PoolAllocator::do_deallocate(void* memory, std::size_t bytes, std::size_t alignment)
{
    _threadResources.Deallocate(memory, bytes, alignment);
}

bool PoolAllocator::do_is_equal(const std::pmr::memory_resource& other) const noexcept
{
    return _resource->is_equal(other);
}

MemoryLogger PoolAllocator::MemoryResource = MemoryLogger(std::pmr::new_delete_resource());

std::shared_ptr<FastTransport::Memory::ThreadSafeAllocator> PoolAllocator::GlobalAllocator = std::make_shared<ThreadSafeAllocator>();

thread_local LocalPoolAllocator PoolAllocator::_threadResources = LocalPoolAllocator(GlobalAllocator, &MemoryResource);

} // namespace FastTransport::Memory