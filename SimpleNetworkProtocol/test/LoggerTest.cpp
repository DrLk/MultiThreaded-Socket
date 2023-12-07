#include "Logger.hpp"
#include "gtest/gtest.h"

#include <chrono>

TEST(LoggerTest, BasicLoggerTest)
{
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    for (int i = 0; i < 10; i++) {
        LOGGER() << i << " " << 123 << "fdsfsd";
    }

    std::cout << clock::now() - start << std::endl;
}