#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

#include "ConnectionContext.hpp"
#include "ISpeedControllerState.hpp"
#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {

class ConnectionContext;

class SpeedController : private ConnectionContext::Subscriber {

    using clock = std::chrono::steady_clock;

    enum class SpeedState : int16_t {
        Fast,
        BBQ,
        Stable,
    };

public:
    SpeedController(const std::shared_ptr<ConnectionContext>& context);
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
    static constexpr std::chrono::seconds QueueTimeInterval = std::chrono::seconds(10);
    static constexpr std::chrono::milliseconds Interval = TimeRangedStats::Interval / 2;

    TimeRangedStats _stats;

    std::map<SpeedState, std::unique_ptr<ISpeedControllerState>> _states;
    SpeedState _currentState = SpeedState::Stable;

    bool _up { true };
    int64_t _speedIncrement { 1 };
    std::atomic<size_t> _minSpeed { 0 };
    std::atomic<size_t> _maxSpeed { 0 };
    std::shared_ptr<ConnectionContext> _context;

    void OnSettingsChanged(const Settings key, size_t value) override;
    void OnMinSpeedChanged(size_t minSpeed);
    void OnMaxSpeedChanged(size_t maxSpeed);
};
} // namespace FastTransport::Protocol
