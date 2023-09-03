#include "SampleStats.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
TEST(SampleStatsTest, SampleStats)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(100, 50, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetAllPackets(), 100);
    EXPECT_EQ(stats.GetLostPackets(), 50);
    EXPECT_EQ(stats.GetStart(), now);
    EXPECT_EQ(stats.GetEnd(), now + 100ms);
    EXPECT_EQ(stats.GetLost(), 50.0);
    EXPECT_EQ(stats.GetRtt(), 20ms);
}

TEST(SampleStatsTest, Merge)
{
    auto now = std::chrono::steady_clock::now();
    SampleStats stats1(100, 50, now, now + 100ms, 20ms);
    const SampleStats stats2(300, 150, now + 50ms, now + 200ms, 10ms);

    stats1.Merge(stats2);

    EXPECT_EQ(stats1.GetAllPackets(), 400);
    EXPECT_EQ(stats1.GetLostPackets(), 200);
    EXPECT_EQ(stats1.GetStart(), now);
    EXPECT_EQ(stats1.GetEnd(), now + 200ms);
    EXPECT_EQ(stats1.GetLost(), 50.0);
    EXPECT_EQ(stats1.GetRtt(), 12500us);
}

TEST(SampleStatsTest, GetAllPackets)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(100, 50, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetAllPackets(), 100);
}

TEST(SampleStatsTest, GetLostPackets)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(100, 50, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetLostPackets(), 50);
}

TEST(SampleStatsTest, GetStart)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(100, 50, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetStart(), now);
}

TEST(SampleStatsTest, GetEnd)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(100, 50, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetEnd(), now + 100ms);
}

TEST(SampleStatsTest, GetLost)
{
    auto now = std::chrono::steady_clock::now();
    const SampleStats stats(200, 100, now, now + 100ms, 20ms);

    EXPECT_EQ(stats.GetLost(), 50.0);
}
} // namespace FastTransport::Protocol