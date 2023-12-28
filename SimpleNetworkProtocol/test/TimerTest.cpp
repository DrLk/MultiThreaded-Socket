#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

TEST(TimerTest, BasicTimerTest)
{
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    // Some computation here
    while (true) {
        auto end = clock::now();

        auto elapsed_seconds = end - start;

        if (elapsed_seconds > 5000ms) {
            break;
        }
    }
}
