#pragma once

#include <algorithm>
#include <chrono>
#include <functional>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

class SpeedController {
    using clock = std::chrono::steady_clock;

public:
    SpeedController();

    size_t GetNumberPacketToSend();
    void UpdateStats(const SampleStats& stats);

private:
    clock::time_point _lastSend {};
    size_t _packetPerSecond {};
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;
    static constexpr std::chrono::seconds QueueTimeInterval = std::chrono::seconds(10);

    std::vector<SampleStats> _stats {};

    bool _up;
    long long _speedIncrement;

    void GetStableSendInterval()
    {
        // std::max_element(_samples.begin(), _samples.end(), std::bind(&Sample::IsBetter, std::placeholders::_1, std::placeholders::_2));
        /*for (Sample& sample : _samples)
        {
            sample.GetRealSpeed();
        }*/
    }
};
} // namespace FastTransport::Protocol
