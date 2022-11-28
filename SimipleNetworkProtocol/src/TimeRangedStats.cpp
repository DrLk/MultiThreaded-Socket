#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {
TimeRangedStats::TimeRangedStats()
{
    auto now = SampleStats::clock::time_point(std::chrono::duration_cast<std::chrono::seconds>(SampleStats::clock::now().time_since_epoch()));
    for (int i = 0; i < StatsSize; i++) {
        _stats.emplace_back(0, 0, now, now + Interval);
        now += Interval;
    }
}

const std::vector<SampleStats>& TimeRangedStats::GetSamplesStats() const
{
    return _stats;
}

void TimeRangedStats::AddPacket(bool lostPacket, SampleStats::clock::time_point time)
{
    const SampleStats stats(1, lostPacket ? 1 : 0, time, time);

    UpdateStats(stats);
}

void TimeRangedStats::UpdateStats(const std::vector<SampleStats>& stats)
{
    for (const SampleStats& stat : stats) {
        UpdateStats(stat);
    }
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

        auto newStartIndex = std::max<size_t>(_startIndex, newEndIndex - Size);
        if (newStartIndex != _startIndex) {
            auto startInterval = _stats[(_startIndex + Size - 1) % Size].GetEnd();
            for (size_t i = _startIndex; i < newStartIndex && i < _startIndex + Size; i++) {
                _stats[i % Size] = SampleStats(0, 0, startInterval, startInterval + Interval);
                startInterval += Interval;
            }

            _startIndex = newStartIndex;
        }

        _stats[newEndIndex % Size].Merge(stats);
    }
}
} // namespace FastTransport::Protocol