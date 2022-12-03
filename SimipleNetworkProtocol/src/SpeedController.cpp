#include "SpeedController.hpp"

#include <algorithm>
#include <limits>
#include <ranges>

#include "ISpeedControllerState.hpp"

namespace FastTransport::Protocol {
using namespace std::chrono_literals;

SpeedController::SpeedController()
    : _packetPerSecond(MinSpeed)
    , _up(true)
    , _speedIncrement(1)
{
    _states = {
        { SpeedState::FAST, new FastAccelerationState() },
        { SpeedState::BBQ, new BBQState() }
    };
}

size_t SpeedController::GetNumberPacketToSend()
{
    auto* state = _states[_currentState];
    static SpeedControllerState speedState;
    state->Run(_stats.GetSamplesStats(), speedState);
    return speedState.realSpeed;
}

void SpeedController::UpdateStats(const TimeRangedStats& stats)
{
    _stats.UpdateStats(stats.GetSamplesStats());
}

SpeedController::clock::duration SpeedController::GetTimeout() const
{
    auto timeout = _stats.GetAverageRtt();
    return timeout + 100ms;
}

} // namespace FastTransport::Protocol