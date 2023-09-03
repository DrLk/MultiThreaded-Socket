#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

using namespace std::chrono_literals;

class TimeRangedStats {
    using clock = std::chrono::steady_clock;

public:
    static constexpr size_t Size = 100;
    static constexpr clock::duration DefaultRTT = 500ms;

    TimeRangedStats();

    [[nodiscard]] const std::vector<SampleStats>& GetSamplesStats() const;
    void AddPacket(bool lostPacket, SampleStats::clock::time_point sendTime, SampleStats::clock::duration rtt);
    void UpdateStats(const TimeRangedStats& stats);

    [[nodiscard]] clock::duration GetAverageRtt() const;

    static constexpr std::chrono::milliseconds Interval = std::chrono::milliseconds(50);

private:
    std::vector<SampleStats> _stats;
    size_t _startIndex = 0;

    void UpdateStats(const SampleStats& stats);
};
} // namespace FastTransport::Protocol