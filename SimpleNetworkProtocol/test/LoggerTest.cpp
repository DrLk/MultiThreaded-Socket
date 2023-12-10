#include "Logger.hpp"
#include "gtest/gtest.h"

TEST(LoggerTest, BasicLoggerTest)
{
    for (int i = 0; i < 10; i++) {
        LOGGER() << i << " " << 123 << "Test Message";
    }
}