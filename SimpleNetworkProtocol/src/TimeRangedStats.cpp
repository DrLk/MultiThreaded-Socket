#include "TimeRangedStats.hpp"

#include <algorithm>
#include <compare>
#include <functional>
#include <numeric>
#include <ratio>

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
TimeRangedStats::TimeRangedStats()
{
    auto now = SampleStats::clock::time_point(std::chrono::duration_cast<std::chrono::seconds>(SampleStats::clock::now().time_since_epoch()));
    for (int i = 0; i < Size; i++) {
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

void TimeRangedStats::UpdateStats(const std::vector<SampleStats>& stats)
{
    for (const SampleStats& stat : stats) {
        UpdateStats(stat);
    }
}

TimeRangedStats::clock::duration TimeRangedStats::GetAverageRtt() const
{
    auto countStats = std::ranges::count_if(_stats, [](const SampleStats& sample) {
        return sample.GetRtt() > 0ms;
    });

    if (countStats < 20) {
        return 500ms;
    }

    auto averageRtt = std::accumulate(_stats.begin(), _stats.end(), clock::duration(), [](clock::duration rtt, const SampleStats& stat) {
        return rtt += stat.GetRtt();
    }) / countStats;

    return averageRtt;
}

void TimeRangedStats::UpdateStats(const SampleStats& stats)
{
    if (stats.GetAllPackets() == 0) {
        return;
    }

    if (_stats[_startIndex % Size].GetStart() > stats.GetStart()) {
        return;
    }

    auto neededStats = std::find_if(_stats.begin(), _stats.end(), [&stats](const SampleStats& stat) {
        return stat.GetStart() <= stats.GetStart() && stat.GetEnd() >= stats.GetEnd();
    });

    if (neededStats != _stats.end()) {
        neededStats->Merge(stats);
    } else {
        auto diff = stats.GetStart() - _stats[_startIndex % Size].GetStart();
        const size_t newEndIndex = _startIndex + diff / Interval;

        auto newStartIndex = std::max<size_t>(_startIndex, newEndIndex - Size + 1);
        auto startInterval = _stats[(_startIndex + Size - 1) % Size].GetEnd();
        for (size_t i = _startIndex; i < newStartIndex && i < _startIndex + Size; i++) {
            _stats[i % Size] = SampleStats(0, 0, startInterval, startInterval + Interval, 0ms);
            startInterval += Interval;
        }

        _startIndex = newStartIndex;

        _stats[newEndIndex % Size].Merge(stats);
    }
}
} // namespace FastTransport::Protocol