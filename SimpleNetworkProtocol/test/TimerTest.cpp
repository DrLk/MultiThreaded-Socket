#include "gtest/gtest.h"

#include <chrono>
#include <compare>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ratio>

using namespace std::chrono_literals;

TEST(TimerTest, BasicTimerTest)
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