#include "SpeedController.hpp"

#include <algorithm>
#include <limits>
#include <ranges>

#include "ISpeedControllerState.hpp"

namespace FastTransport::Protocol {
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

void SpeedController::UpdateStats(const RangedList& stats)
{
    _stats.UpdateStats(stats.GetSamplesStats());
}

} // namespace FastTransport::Protocol