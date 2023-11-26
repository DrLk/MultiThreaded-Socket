#include "LocalPoolAllocator.hpp"

#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {

LocalPoolAllocator::LocalPoolAllocator(const std::shared_ptr<ThreadSafeAllocator>& globalAllocator, std::pmr::memory_resource* resource)
    : _resource(resource)
    , _globalAllocator(globalAllocator)
{
}

LocalPoolAllocator::~LocalPoolAllocator() = default;

void* LocalPoolAllocator::Allocate(std::size_t size, std::size_t alignment)
{
    auto free = _pool.find({ size, alignment });
    if (free != _pool.end()) {
        void* allocation = free->second;
        _pool.erase(free);
        return allocation;
    }

    std::size_t count = MaxFreeSize;
    for (; count != 0; count--) {
        auto allocation = _globalAllocator->Allocate(size, alignment);
        if (allocation.empty()) {
            break;
        }

        _pool.insert({ std::move(allocation) });
    }

    auto allocation = _globalAllocator->Allocate(size, alignment);
    if (!allocation.empty()) {
        return allocation.mapped();
    }

    while (count != 0) {
        void* bytes = _resource->allocate(size, alignment);
        _pool.insert({ { size, alignment }, bytes });
        count--;
    }

    return _resource->allocate(size, alignment);
}

void LocalPoolAllocator::Deallocate(void* allocation, std::size_t size, std::size_t alignment)
{
    auto bucket = _pool.bucket({ size, alignment });
    std::size_t bucketSize = _pool.bucket_size(bucket);
    if (bucketSize < MaxFreeSize * 2) {
        _pool.insert({ { size, alignment }, allocation });
    } else {
        for (int i = 0; i < MaxFreeSize; i++) {
            AllocationNode allocation = _pool.extract({ size, alignment });
            _globalAllocator->Deallocate(std::move(allocation));
        }
    }
}

} // namespace FastTransport::Memory