#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <map>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

class ISpeedControllerState;

class SpeedController {
    using clock = std::chrono::steady_clock;

    enum class SpeedState : short {
        FAST
    };

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

    std::list<SampleStats> _stats {};

    std::map<SpeedState, ISpeedControllerState*> _states = {};
    SpeedState _currentState = SpeedState::FAST;

    bool _up;
    long long _speedIncrement;
};
} // namespace FastTransport::Protocol
