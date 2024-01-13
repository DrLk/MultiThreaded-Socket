#include <gtest/gtest.h>

#include <cstddef>
#include <list>
#include <memory>
#include <memory_resource>
#include <thread>

#include "MemoryLogger.hpp"
#include "PoolAllocator.hpp"

namespace FastTransport::Memory {

TEST(MemoryTest, List)
{
    auto resource = std::pmr::unsynchronized_pool_resource(std::pmr::new_delete_resource());
    MemoryLogger memory_resource(&resource);

    const std::pmr::polymorphic_allocator<int> allocator(&memory_resource);
    std::pmr::list<int> list(allocator);

    static constexpr size_t ListSize = 1000;
    std::pmr::list<int> preallocated_list(allocator);
    for (int i = 0; i < ListSize; ++i) {
        preallocated_list.push_back(i);
    }

    auto iterator = preallocated_list.begin();
    for (size_t i = 0; i < ListSize / 2; ++i) {
        iterator++;
    }

    list.splice(list.begin(), preallocated_list, preallocated_list.begin(), iterator);
}

TEST(MemoryTest, MemoryAllocator)
{
    auto memoryLogger = std::make_unique<MemoryLogger>(std::pmr::new_delete_resource());
    PoolAllocator allocator(memoryLogger.get());

    std::jthread thread1([&allocator]() {
        void* address = allocator.allocate(100);
        allocator.deallocate(address, 100);
    });

    thread1.join();
    EXPECT_TRUE(memoryLogger->GetAllocatedSize() == 100);

    std::jthread thread2([&allocator]() {
        void* address = allocator.allocate(100);
        allocator.deallocate(address, 100);
    });

    thread2.join();
    EXPECT_TRUE(memoryLogger->GetAllocatedSize() == 100);
}

TEST(MemoryTest, PoolList)
{
    auto memoryLogger = std::make_unique<MemoryLogger>(std::pmr::new_delete_resource());
    PoolAllocator allocator(memoryLogger.get());

    std::pmr::list<int> list(&allocator);

    for (int i = 0; i < 10000; i++) {
        list.push_back(i);
    }

    auto allocatedSize = memoryLogger->GetAllocatedSize();

    for (int i = 0; i < 10000; i++) {
        list.pop_back();
    }
    EXPECT_TRUE(memoryLogger->GetAllocatedSize() == allocatedSize);

    for (int i = 0; i < 10000; i++) {
        list.push_back(i);
    }
    EXPECT_TRUE(memoryLogger->GetAllocatedSize() == allocatedSize);

    for (int i = 0; i < 10000; i++) {
        list.pop_back();
        list.push_back(i);
    }
    EXPECT_TRUE(memoryLogger->GetAllocatedSize() == allocatedSize);
}

} // namespace FastTransport::Memory
