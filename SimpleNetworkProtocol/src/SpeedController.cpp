#include "SpeedController.hpp"

#include <compare>
#include <ratio>

namespace FastTransport::Protocol {
using namespace std::chrono_literals;

SpeedController::SpeedController()
    : _lastSend(clock::now())
    , _packetPerSecond(MinSpeed)
    , _up(true)
    , _speedIncrement(1)
{
    _states.emplace(SpeedState::FAST, std::make_unique<FastAccelerationState>());
    _states.emplace(SpeedState::BBQ, std::make_unique<BBQState>());
    _states.emplace(SpeedState::STABLE, std::make_unique<StableState>());
}

size_t SpeedController::GetNumberPacketToSend()
{
    auto now = clock::now();
    auto diff = now - _lastSend;

    if (diff < (TimeRangedStats::Interval / 2)) {
        return 0;
    }

    const auto& state = _states[_currentState];
    static SpeedControllerState speedState;
    state->Run(_stats.GetSamplesStats(), speedState);

    const size_t coeficient = 100s / diff;
    if (coeficient != 0) {
        const size_t ration = 1s / TimeRangedStats::Interval;

        const size_t number = speedState.realSpeed * ration * 100 / coeficient;
        if (number != 0) {
            _lastSend = now;
        }

        return number;
    }

    return 0;
}

void SpeedController::UpdateStats(const TimeRangedStats& stats)
{
    _stats.UpdateStats(stats.GetSamplesStats());
}

SpeedController::clock::duration SpeedController::GetTimeout() const
{
    auto timeout = _stats.GetAverageRtt();
    return timeout + 50ms;
}

} // namespace FastTransport::Protocol