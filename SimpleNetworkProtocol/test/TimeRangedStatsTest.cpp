#include "TimeRangedStats.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <compare>
#include <functional>
#include <numeric>
#include <ratio>

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
TEST(TimeRangedStatsTest, TimeRangedStats)
{
    TimeRangedStats stats;

    const std::vector<SampleStats>& samples = stats.GetSamplesStats();
    EXPECT_EQ(samples.size(), TimeRangedStats::Size);
    EXPECT_EQ(stats.GetAverageRtt(), TimeRangedStats::DefaultRTT);

    for (size_t i = 0; i < TimeRangedStats::Size; i++) {
        EXPECT_EQ(samples[i].GetAllPackets(), 0);
        EXPECT_EQ(samples[i].GetLostPackets(), 0);
        EXPECT_EQ(samples[i].GetStart() - samples[0].GetStart(), i * TimeRangedStats::Interval);
        EXPECT_EQ(samples[i].GetEnd() - samples[0].GetStart(), (i + 1) * TimeRangedStats::Interval);
        EXPECT_EQ(samples[i].GetLost(), 0.0);
        EXPECT_EQ(samples[i].GetRtt(), 0ms);
    }
}

TEST(TimeRangedStatsTest, AddPacket)
{
    TimeRangedStats stats;

    auto now = std::chrono::steady_clock::now();
    stats.AddPacket(false, now, 10ms);

    const std::vector<SampleStats>& samples = stats.GetSamplesStats();

    auto sample = std::find_if(samples.begin(), samples.end(), [now](const SampleStats& sample) {
        return sample.GetStart() <= now && sample.GetEnd() >= now;
    });

    EXPECT_EQ(sample->GetAllPackets(), 1);
    EXPECT_EQ(sample->GetLostPackets(), 0);
    EXPECT_LE(sample->GetStart(), now);
    EXPECT_GE(sample->GetEnd(), now);
    EXPECT_EQ(sample->GetLost(), 0.0);
    EXPECT_EQ(sample->GetRtt(), 10ms);
}

TEST(TimeRangedStatsTest, AddPacket2)
{
    TimeRangedStats stats;

    auto now = std::chrono::steady_clock::now();
    stats.AddPacket(false, now, 10ms);
    stats.AddPacket(false, now + TimeRangedStats::Interval, 20ms);
    stats.AddPacket(false, now + 2 * TimeRangedStats::Interval, 30ms);
    stats.AddPacket(false, now + 2 * TimeRangedStats::Interval, 30ms);

    const std::vector<SampleStats>& samples = stats.GetSamplesStats();

    auto sample = std::find_if(samples.begin(), samples.end(), [now](const SampleStats& sample) {
        return sample.GetStart() <= now && sample.GetEnd() >= now;
    });

    EXPECT_EQ(sample->GetAllPackets(), 1);
    EXPECT_EQ(sample->GetLostPackets(), 0);
    EXPECT_EQ(sample->GetLost(), 0.0);
    EXPECT_EQ(sample->GetRtt(), 10ms);

    sample++;
    EXPECT_EQ(sample->GetAllPackets(), 1);
    EXPECT_EQ(sample->GetLostPackets(), 0);
    EXPECT_LE(sample->GetStart(), now + TimeRangedStats::Interval);
    EXPECT_GE(sample->GetEnd(), now + TimeRangedStats::Interval);
    EXPECT_EQ(sample->GetLost(), 0.0);
    EXPECT_EQ(sample->GetRtt(), 20ms);

    sample++;
    EXPECT_EQ(sample->GetAllPackets(), 2);
    EXPECT_EQ(sample->GetLostPackets(), 0);
    EXPECT_LE(sample->GetStart(), now + 2 * TimeRangedStats::Interval);
    EXPECT_GE(sample->GetEnd(), now + 2 * TimeRangedStats::Interval);
    EXPECT_EQ(sample->GetLost(), 0.0);
    EXPECT_EQ(sample->GetRtt(), 30ms);
}

TEST(TimeRangedStatsTest, UpdateStats1)
{
    TimeRangedStats stats1;
    auto now = std::chrono::steady_clock::now();
    stats1.AddPacket(false, now, 10ms);

    TimeRangedStats stats2;
    stats2.AddPacket(false, now + 10s, 20ms);

    TimeRangedStats stats3;
    stats3.AddPacket(false, now + 20s, 30ms);

    TimeRangedStats stats4;
    stats4.AddPacket(true, now + 30s, 40ms);

    TimeRangedStats stats5;
    stats5.AddPacket(false, now + 40s, 50ms);

    TimeRangedStats stats6;
    stats6.AddPacket(true, now + 50s, 60ms);

    TimeRangedStats stats7;
    stats7.AddPacket(false, now + 60s, 70ms);

    TimeRangedStats stats8;
    stats8.AddPacket(true, now + 70s, 80ms);

    TimeRangedStats stats9;
    stats9.AddPacket(false, now + 80s, 90ms);

    TimeRangedStats stats10;
    stats10.AddPacket(true, now + 90s, 100ms);

    stats1.UpdateStats(stats2);
    stats1.UpdateStats(stats3);
    stats1.UpdateStats(stats4);
    stats1.UpdateStats(stats5);
    stats1.UpdateStats(stats6);
    stats1.UpdateStats(stats7);
    stats1.UpdateStats(stats8);
    stats1.UpdateStats(stats9);
    stats1.UpdateStats(stats10);

    const std::vector<SampleStats>& samples = stats1.GetSamplesStats();

    int result = std::accumulate(samples.begin(), samples.end(), 0, [](int accumulator, const SampleStats& sample) {
        return accumulator + sample.GetAllPackets();
    });

    EXPECT_EQ(result, 1);
    EXPECT_EQ(stats1.GetAverageRtt(), TimeRangedStats::DefaultRTT);
}

TEST(TimeRangedStatsTest, UpdateStats2)
{
    TimeRangedStats finalStats;
    auto now = std::chrono::steady_clock::now();

    const int packetCount = 200;
    auto packetInterval = TimeRangedStats::Interval / 200;

    for (int i = 0; i < packetCount; i++) {
        TimeRangedStats stats;
        stats.AddPacket(i % 2 == 1, now + i * packetInterval, 10ms * i);

        finalStats.UpdateStats(stats);
    }

    const std::vector<SampleStats>& samples = finalStats.GetSamplesStats();

    int result = std::accumulate(samples.begin(), samples.end(), 0, [](int accumulator, const SampleStats& sample) {
        return accumulator + sample.GetAllPackets();
    });

    auto sample = std::find_if(samples.begin(), samples.end(), [packetCount](const SampleStats& sample) {
        return sample.GetAllPackets() == packetCount;
    });

    EXPECT_EQ(result, packetCount);
    EXPECT_EQ(sample->GetAllPackets(), packetCount);
    EXPECT_EQ(sample->GetLostPackets(), packetCount / 2);
    EXPECT_EQ(sample->GetLost(), 50.0);
    EXPECT_EQ(finalStats.GetAverageRtt(), 500ms);
}

}
