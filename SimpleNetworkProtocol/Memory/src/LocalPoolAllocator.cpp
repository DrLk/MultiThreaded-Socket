#include "LocalPoolAllocator.hpp"

#include "ThreadSafeAllocator.hpp"

namespace FastTransport::Memory {

LocalPoolAllocator::LocalPoolAllocator(const std::shared_ptr<ThreadSafeAllocator>& globalAllocator)
    : _globalAllocator(globalAllocator)
{
}

LocalPoolAllocator::~LocalPoolAllocator()
{
    _globalAllocator->Deallocate(std::move(_pool));
}

void* LocalPoolAllocator::Allocate(std::size_t size, std::size_t alignment)
{
    auto& backetSize = _backetSize[{ size, alignment }];
    auto free = _pool.find({ size, alignment });
    if (free != _pool.end()) {
        void* allocation = free->second;
        _pool.erase(free);
        backetSize--;
        return allocation;
    }

    auto nodes = _globalAllocator->Allocate2(size, alignment);

    backetSize = 0;
    for (auto&& node : nodes) {
        if (node.empty()) {
            break;
        }

        free = _pool.insert({ std::move(node) });
        backetSize++;
    }

    if (free != _pool.end()) {

        void* allocation = free->second;
        _pool.erase(free);
        backetSize--;
        return allocation;
    }

    return _globalAllocator->Allocate(size, alignment);
}

void LocalPoolAllocator::Deallocate(void* memory, std::size_t size, std::size_t alignment)
{
    auto& bucketSize = _backetSize[{ size, alignment }];

    if (bucketSize > MaxFreeSize * 2) {
        ThreadSafeAllocator::AllocationNodes nodes;
        auto [begin, end] = _pool.equal_range({ size, alignment });
        int index = 0;
        for (auto iterator = begin; iterator != end && index < nodes.size(); ++index) {
            nodes.at(index) = _pool.extract(iterator++);
            bucketSize--;
        }

        _globalAllocator->Deallocate(std::move(nodes));
    }

    bucketSize++;
    _pool.insert({ { size, alignment }, memory });
}

} // namespace FastTransport::Memory
