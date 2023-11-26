#include "gtest/gtest.h"

#include <list>
#include <memory>
#include <memory_resource>
#include <thread>

#include "Logger.hpp"
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

    // list.splice(list.begin(), preallocated_list);

    /*for(int i = 0; i < ListSize; ++i) {
        preallocated_list.pop_front();
    }*/

    auto iterator = preallocated_list.begin();
    for (size_t i = 0; i < ListSize / 2; ++i) {
        iterator++;
    }

    list.splice(list.begin(), preallocated_list, preallocated_list.begin(), iterator);
}

TEST(MemoryTest, MemoryAllocator)
{
    PoolAllocator allocator;

    std::jthread thread1([&allocator]() {
        void* address = allocator.allocate(100);
    });

    thread1.join();
}

TEST(MemoryTest, PoolList)
{
    PoolAllocator allocator;

    std::pmr::list<int> list(&allocator);

    for (int i = 0; i < 100000; i++) {
        list.push_back(i);
    }

    for (int i = 0; i < 100000; i++) {
        list.pop_back();
    }

    for (int i = 0; i < 50000; i++) {
        list.push_back(i);
    }

    for (int i = 0; i < 1000000; i++) {
        list.pop_back();
        list.push_back(i);
    }
}

} // namespace FastTransport::Memory