#pragma once

#include <algorithm>
#include <chrono>

namespace FastTransport::Protocol {

struct SampleStats {
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

    int allPackets;
    int lostPackets;
    clock::time_point start;
    clock::time_point end;

    int speed {};
    float lost {};

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

private:
    void CalcStats()
    {
        lost = (lostPackets * 100.0) / allPackets;

        auto duration = std::chrono::time_point_cast<std::chrono::microseconds>(end) - std::chrono::time_point_cast<std::chrono::microseconds>(start);
        speed = allPackets;
        speed = speed * (1000000 / duration.count());
        if (speed < 0) {
            int a = 0;
            a++;
        }
    }
};
} // namespace FastTransport::Protocol