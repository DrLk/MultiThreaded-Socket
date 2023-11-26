#include "MemoryLogger.hpp"

#include "Logger.hpp"

namespace FastTransport::Memory {
MemoryLogger::MemoryLogger(std::pmr::memory_resource* resource)
    : _resource(resource)
    , _allocatedSize(0)
{
}

std::size_t MemoryLogger::GetAllocatedSize() const
{
    return _allocatedSize;
}

void* MemoryLogger::do_allocate(std::size_t size, std::size_t alignment)
{
    _allocatedSize += size;
    LOGGER() << " allocate " << size << " bytes";
    return _resource->allocate(size, alignment);
}

void MemoryLogger::do_deallocate(void* memory, std::size_t bytes, std::size_t alignment)
{
    _allocatedSize -= bytes;
    LOGGER() << "deallocate " << bytes << " bytes";
    _resource->deallocate(memory, bytes, alignment);
}

bool MemoryLogger::do_is_equal(const std::pmr::memory_resource& other) const noexcept
{
    LOGGER() << "is_equal";
    return _resource->is_equal(other);
}
} // namespace FastTransport::Memory