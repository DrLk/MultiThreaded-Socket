#pragma once

#include <chrono>
#include <functional>
#include <map>

#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {

class ISpeedControllerState;

class SpeedController {
    using clock = std::chrono::steady_clock;

    enum class SpeedState : int16_t {
        FAST,
        BBQ
    };

public:
    SpeedController();

    size_t GetNumberPacketToSend();
    void UpdateStats(const TimeRangedStats& stats);
    [[nodiscard]] clock::duration GetTimeout() const;

private:
    clock::time_point _lastSend {};
    size_t _packetPerSecond {};
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;
    static constexpr std::chrono::seconds QueueTimeInterval = std::chrono::seconds(10);

    TimeRangedStats _stats;

    std::map<SpeedState, ISpeedControllerState*> _states = {};
    SpeedState _currentState = SpeedState::BBQ;

    bool _up;
    int64_t _speedIncrement;
};
} // namespace FastTransport::Protocol
