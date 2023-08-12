#pragma once

#include <chrono>
#include <stddef.h>
#include <vector>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {
class TimeRangedStats {
    using clock = std::chrono::steady_clock;

public:
    TimeRangedStats();

    [[nodiscard]] const std::vector<SampleStats>& GetSamplesStats() const;
    void AddPacket(bool lostPacket, SampleStats::clock::time_point sendTime, SampleStats::clock::duration rtt);
    void UpdateStats(const std::vector<SampleStats>& stats);

    [[nodiscard]] clock::duration GetAverageRtt() const;

    static constexpr std::chrono::milliseconds Interval = std::chrono::milliseconds(50);

private:
    static constexpr size_t Size = 100;

    std::vector<SampleStats> _stats;
    size_t _startIndex = 0;

    void UpdateStats(const SampleStats& stats);
};
} // namespace FastTransport::Protocol