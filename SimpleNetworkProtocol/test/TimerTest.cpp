#include "gtest/gtest.h"

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

TEST(TimerTest, BasicTimerTest)
//void m3()
{
    uint64_t counter = 0;
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    // Some computation here
    while (true) {
        counter++;
        auto end = clock::now();

        auto elapsed_seconds = end - start;

        if (elapsed_seconds > 5000ms) {
            break;
        }
    }

    std::cout << "counter: " << counter << std::endl;
}