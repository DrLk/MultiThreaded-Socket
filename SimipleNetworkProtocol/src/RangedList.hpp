#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {
class RangedList {
    using clock = std::chrono::steady_clock;

    static constexpr int StatsSize = 100;
    static constexpr std::chrono::milliseconds Interval = std::chrono::milliseconds(100);
    static constexpr size_t Size = 100;

    std::vector<SampleStats> _stats;
    int _startIndex = 0;
    clock::time_point _beginTime;

public:
    RangedList();

    [[nodiscard]] const std::vector<SampleStats>& GetSamplesStats() const;

    void AddPacket(bool lostPacket, SampleStats::clock::time_point time);

    void UpdateStats(const std::vector<SampleStats>& stats);
    void UpdateStats(const SampleStats& stats);
};
} // namespace FastTransport::Protocol