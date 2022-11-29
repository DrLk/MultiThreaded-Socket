#pragma once

#include <algorithm>
#include <chrono>

namespace FastTransport::Protocol {

struct alignas(32) SampleStats {
    using clock = std::chrono::steady_clock;

    static constexpr int MinPacketsCount = 100;

    SampleStats(int allPackets, int lostPackets, clock::time_point start, clock::time_point end)
        : allPackets(allPackets)
        , lostPackets(lostPackets)
        , start(start)
        , end(end)
    {
        if (allPackets > MinPacketsCount) {
            CalcStats();
        }
    }

    void Merge(const SampleStats& stats)
    {
        allPackets += stats.allPackets;
        lostPackets += stats.lostPackets;
        start = std::min<clock::time_point>(start, stats.start);
        end = std::max<clock::time_point>(end, stats.end);

        if (allPackets > MinPacketsCount) {
            CalcStats();
        }
    }

    [[nodiscard]] int GetAllPackets() const
    {
        return allPackets;
    }

    [[nodiscard]] int GetLostPackets() const
    {
        return lostPackets;
    }

    [[nodiscard]] clock::time_point GetStart() const
    {
        return start;
    }

    [[nodiscard]] clock::time_point GetEnd() const
    {
        return end;
    }

    [[nodiscard]] double GetLost() const
    {
        return lost;
    }

private:
    int allPackets;
    int lostPackets;

    clock::time_point start;
    clock::time_point end;

    double lost {};

    void CalcStats()
    {
        lost = (lostPackets * 100.0) / allPackets;
    }
};
} // namespace FastTransport::Protocol