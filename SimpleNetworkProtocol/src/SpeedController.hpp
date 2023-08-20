#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

#include "ISpeedControllerState.hpp"
#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {
class SpeedController {

    using clock = std::chrono::steady_clock;

    enum class SpeedState : int16_t {
        FAST,
        BBQ,
        STABLE,
    };

public:
    SpeedController();
    SpeedController(const SpeedController& that) = delete;
    SpeedController(SpeedController&& that) = delete;
    SpeedController& operator=(const SpeedController& that) = delete;
    SpeedController& operator=(SpeedController&& that) = delete;
    ~SpeedController() = default;

    size_t GetNumberPacketToSend();
    void UpdateStats(const TimeRangedStats& stats);
    [[nodiscard]] clock::duration GetTimeout() const;

private:
    clock::time_point _lastSend;
    size_t _packetPerSecond;
    static constexpr size_t MinSpeed = 1000;
    static constexpr size_t MaxSpeed = 10000;
    static constexpr std::chrono::seconds QueueTimeInterval = std::chrono::seconds(10);

    TimeRangedStats _stats;

    std::map<SpeedState, std::unique_ptr<ISpeedControllerState>> _states;
    SpeedState _currentState = SpeedState::STABLE;

    bool _up { true };
    int64_t _speedIncrement { 1 };
};
} // namespace FastTransport::Protocol
