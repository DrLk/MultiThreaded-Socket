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
    _states = { { SpeedState::FAST, new FastAccelerationState() } };
}

size_t SpeedController::GetNumberPacketToSend()
{
    auto realSpeedStats = _stats.GetSamplesStats() | std::views::filter([](const SampleStats& sample) {
        return sample.lost < 5;
    });

    auto maxRealSpeedIterator = std::ranges::max_element(realSpeedStats, {}, &SampleStats::speed);
    int maxRealSpeed = 1;
    if (maxRealSpeedIterator != realSpeedStats.end()) {
        maxRealSpeed = maxRealSpeedIterator.base()->speed;
    }

    auto lostSpeedStats = _stats.GetSamplesStats() | std::views::filter([](const SampleStats& sample) {
        return sample.lost >= 5;
    });

    auto minLostSpeedIterator = std::ranges::min_element(lostSpeedStats, {}, &SampleStats::speed);
    int minLostSpeed = std::numeric_limits<int>::max();
    if (minLostSpeedIterator != lostSpeedStats.end()) {
        minLostSpeed = minLostSpeedIterator.base()->speed;
    }

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