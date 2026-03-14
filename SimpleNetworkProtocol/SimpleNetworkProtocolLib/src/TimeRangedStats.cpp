#include "TimeRangedStats.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <numeric>
#include <vector>

#include "SampleStats.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
TimeRangedStats::TimeRangedStats()
{
    auto now = SampleStats::clock::time_point(std::chrono::duration_cast<std::chrono::seconds>(SampleStats::clock::now().time_since_epoch()));
    for (size_t i = 0; i < Size; i++) {
        _stats.emplace_back(0, 0, now, now + Interval, 0ms);
        now += Interval;
    }
}

const std::vector<SampleStats>& TimeRangedStats::GetSamplesStats() const
{
    return _stats;
}

void TimeRangedStats::AddPacket(bool lostPacket, SampleStats::clock::time_point sendTime, SampleStats::clock::duration rtt)
{
    const SampleStats stats(1, lostPacket ? 1 : 0, sendTime, sendTime, rtt);

    UpdateStats(stats);
}

void TimeRangedStats::UpdateStats(const TimeRangedStats& stats)
{
    for (const SampleStats& stat : stats._stats) {
        UpdateStats(stat);
    }
}

TimeRangedStats::clock::duration TimeRangedStats::GetAverageRtt() const
{
    auto countStats = std::ranges::count_if(_stats, [](const SampleStats& sample) {
        return sample.GetRtt() > 0ms;
    });

    if (countStats < 20) {
        return DefaultRTT;
    }

    auto averageRtt = std::accumulate(_stats.begin(), _stats.end(), clock::duration(), [](clock::duration rtt, const SampleStats& stat) {
        return rtt += stat.GetRtt();
    }) / countStats;

    return averageRtt;
}

TimeRangedStats::clock::duration TimeRangedStats::GetMaxRtt() const
{
    auto countStats = std::ranges::count_if(_stats, [](const SampleStats& sample) {
        return sample.GetRtt() > 0ms;
    });

    if (countStats < 20) {
        return DefaultRTT;
    }

    auto maxRtt = std::ranges::max_element(_stats,
        [](const SampleStats& left, const SampleStats& right) {
            return left.GetRtt() < right.GetRtt();
        });

    return maxRtt->GetRtt();
}

void TimeRangedStats::UpdateStats(const SampleStats& stats)
{
    if (stats.GetAllPackets() == 0) {
        return;
    }

    const auto windowStart = _stats[_startIndex % Size].GetStart();
    if (windowStart > stats.GetStart()) {
        return;
    }

    const auto diff = stats.GetStart() - windowStart;
    const size_t slotIndex = _startIndex + diff / Interval;

    if (slotIndex < _startIndex + Size) {
        _stats[slotIndex % Size].Merge(stats);
        return;
    }

    const auto newStartIndex = std::max<size_t>(_startIndex, slotIndex - Size + 1);
    auto startInterval = _stats[(_startIndex + Size - 1) % Size].GetEnd();
    for (size_t i = _startIndex; i < newStartIndex && i < _startIndex + Size; i++) {
        _stats[i % Size] = SampleStats(0, 0, startInterval, startInterval + Interval, 0ms);
        startInterval += Interval;
    }

    _startIndex = newStartIndex;

    const auto slotStart = windowStart + (diff / Interval) * Interval;
    _stats[slotIndex % Size] = SampleStats(0, 0, slotStart, slotStart + Interval, 0ms);
    _stats[slotIndex % Size].Merge(stats);
}
} // namespace FastTransport::Protocol
