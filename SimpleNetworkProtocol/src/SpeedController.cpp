#include "SpeedController.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <map>
#include <memory>

#include "ISpeedControllerState.hpp"
#include "TimeRangedStats.hpp"
#include <ConnectionContext.hpp>

namespace FastTransport::Protocol {
using namespace std::chrono_literals;

SpeedController::SpeedController(const std::shared_ptr<ConnectionContext>& context)
    : ConnectionContext::Subscriber(context)
    , _lastSend(clock::now())
    , _context(context)
{
    _context->Subscribe(*this);

    _states.emplace(SpeedState::Fast, std::make_unique<FastAccelerationState>());
    _states.emplace(SpeedState::BBQ, std::make_unique<BBQState>());
    _states.emplace(SpeedState::Stable, std::make_unique<StableState>());
}

size_t SpeedController::GetNumberPacketToSend()
{
    auto now = clock::now();
    auto diff = now - _lastSend;

    if (diff < Interval) {
        return 0;
    }

    const auto& state = _states[_currentState];
    static SpeedControllerState speedState;
    state->Run(_stats, speedState);

    const size_t coeficient = 100s / diff;
    if (coeficient != 0) {
        const size_t ration = 1s / TimeRangedStats::Interval;

        const size_t realSpeed = std::clamp<size_t>(speedState.realSpeed, _minSpeed, _maxSpeed);
        const size_t number = realSpeed * ration * 100 / coeficient;
        if (number != 0) {
            _lastSend = now;
        }

        return number;
    }

    return 0;
}

void SpeedController::UpdateStats(const TimeRangedStats& stats)
{
    _stats.UpdateStats(stats);
}

SpeedController::clock::duration SpeedController::GetTimeout() const
{
    auto timeout = _stats.GetMaxRtt();
    return timeout + 50ms;
}

void SpeedController::OnSettingsChanged(Settings key, size_t value)
{
    switch (key) {
    case Settings::MinSpeed:
        OnMinSpeedChanged(value);
        break;
    case Settings::MaxSpeed:
        OnMaxSpeedChanged(value);
        break;
    default:
        break;
    }
}

void SpeedController::OnMinSpeedChanged(size_t minSpeed)
{
    _minSpeed = minSpeed;
}

void SpeedController::OnMaxSpeedChanged(size_t maxSpeed)
{
    _maxSpeed = maxSpeed;
}

} // namespace FastTransport::Protocol
