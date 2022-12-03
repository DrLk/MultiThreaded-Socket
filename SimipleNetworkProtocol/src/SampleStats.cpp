#include "Sample.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
SampleStats::SampleStats(int allPackets, int lostPackets, clock::time_point start, clock::time_point end, clock::duration rtt)
    : _allPackets(allPackets)
    , _lostPackets(lostPackets)
    , _start(start)
    , _end(end)
    , _rtt(rtt)
    , _lost(0)
{
    if (allPackets > MinPacketsCount) {
        CalcStats();
    }
}

void SampleStats::Merge(const SampleStats& stats)
{
    _allPackets += stats._allPackets;
    _lostPackets += stats._lostPackets;
    _start = std::min<clock::time_point>(_start, stats._start);
    _end = std::max<clock::time_point>(_end, stats._end);

    if (_allPackets > MinPacketsCount) {
        CalcStats();
    }
}

int SampleStats::GetAllPackets() const
{
    return _allPackets;
}

int SampleStats::GetLostPackets() const
{
    return _lostPackets;
}

SampleStats::clock::time_point SampleStats::GetStart() const
{
    return _start;
}

SampleStats::clock::time_point SampleStats::GetEnd() const
{
    return _end;
}

double SampleStats::GetLost() const
{
    return _lost;
}

SampleStats::clock::duration SampleStats::GetRtt() const
{
    return _rtt;
}

void SampleStats::CalcStats()
{
    _lost = (_lostPackets * 100.0) / _allPackets;
}
}
