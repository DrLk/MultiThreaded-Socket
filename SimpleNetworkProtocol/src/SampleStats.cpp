
#include <algorithm>
#include <chrono>
#include <ratio>

#include "SampleStats.hpp"

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

void SampleStats::Merge(const SampleStats& that)
{
    _start = std::min<clock::time_point>(_start, that._start);
    _end = std::max<clock::time_point>(_end, that._end);

    const int thatAcknowledgedPackets = that._allPackets - that._lostPackets;
    if (thatAcknowledgedPackets != 0) {
        const int acknowledgedPackets = _allPackets - _lostPackets;
        _rtt = (_rtt * acknowledgedPackets + that._rtt * thatAcknowledgedPackets) / (thatAcknowledgedPackets + acknowledgedPackets);
    }

    _allPackets += that._allPackets;
    _lostPackets += that._lostPackets;

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
} // namespace FastTransport::Protocol
