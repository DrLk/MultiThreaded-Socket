#include "PoolAllocator.hpp"

#include <cassert>

#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {

PoolAllocator::PoolAllocator(std::pmr::memory_resource* resource)
    : _globalAllocator(std::make_shared<ThreadSafeAllocator>(resource))

{
}

void* PoolAllocator::do_allocate(std::size_t size, std::size_t alignment)
{
    return GetThreadPool().Allocate(size, alignment);
}

void PoolAllocator::do_deallocate(void* memory, std::size_t bytes, std::size_t alignment)
{
    GetThreadPool().Deallocate(memory, bytes, alignment);
}

bool PoolAllocator::do_is_equal(const std::pmr::memory_resource&  /*other*/) const noexcept
{
    assert(false);
    return false;
}

LocalPoolAllocator& PoolAllocator::GetThreadPool()
{
    thread_local LocalPoolAllocator threadResources(_globalAllocator);

    return threadResources;
}

} // namespace FastTransport::Memory
