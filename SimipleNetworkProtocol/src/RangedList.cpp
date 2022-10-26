#include "RangedList.hpp"

namespace FastTransport::Protocol {
RangedList::RangedList()
{
    auto now = SampleStats::clock::time_point(std::chrono::duration_cast<std::chrono::seconds>(SampleStats::clock::now().time_since_epoch()));
    for (int i = 0; i < StatsSize; i++) {
        _stats.emplace_back(1, 0, now, now + Interval);
        now += Interval;
    }
}

const std::vector<SampleStats>& RangedList::GetSamplesStats() const
{
    return _stats;
}

void RangedList::AddPacket(bool lostPacket, SampleStats::clock::time_point time)
{
    SampleStats stats(1, lostPacket ? 1 : 0, time, time);

    UpdateStats(stats);
}

void RangedList::UpdateStats(const std::vector<SampleStats>& stats)
{
    for (const SampleStats& stat : stats) {
        UpdateStats(stat);
    }
}

void RangedList::UpdateStats(const SampleStats& stats)
{
    auto neededStats = std::find_if(_stats.begin(), _stats.end(), [&stats](const SampleStats& it) {
        return it.start <= stats.start && it.end >= stats.end;
    });

    if (neededStats != _stats.end()) {
        neededStats->Merge(stats);
    }

    if (_stats[_startIndex % Size].start > stats.start) {
        return;
    }

    auto diff = stats.start - _stats[_startIndex % Size].start;
    int nextStart = _startIndex + diff / Interval;

    auto newStartIndex = std::max<int>(_startIndex, nextStart - Size + 1);
    if (newStartIndex != _startIndex) {
        for (int i = _startIndex; i < newStartIndex && i < Size; i++) {
            auto startInterval = _stats[(_startIndex + Size - 1) % Size].end;
            _stats[i % Size] = SampleStats(0, 0, startInterval, startInterval + Interval);
            startInterval += Interval;
        }

        _startIndex = newStartIndex;
    }

    auto index = nextStart % Size;

    _stats[index].Merge(stats);
    return;
}
} // namespace FastTransport::Protocol