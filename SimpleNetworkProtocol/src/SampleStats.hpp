#pragma once

#include <chrono>

namespace FastTransport::Protocol {

struct alignas(64) SampleStats {
    using clock = std::chrono::steady_clock;

    static constexpr int MinPacketsCount = 100;

    SampleStats(int allPackets, int lostPackets, clock::time_point start, clock::time_point end, clock::duration rtt);

    void Merge(const SampleStats& that);

    [[nodiscard]] int GetAllPackets() const;
    [[nodiscard]] int GetLostPackets() const;
    [[nodiscard]] clock::time_point GetStart() const;

    [[nodiscard]] clock::time_point GetEnd() const;
    [[nodiscard]] double GetLost() const;
    [[nodiscard]] clock::duration GetRtt() const;

private:
    int _allPackets;
    int _lostPackets;

    clock::time_point _start;
    clock::time_point _end;
    clock::duration _rtt;

    double _lost;

    void CalcStats();
};
} // namespace FastTransport::Protocol