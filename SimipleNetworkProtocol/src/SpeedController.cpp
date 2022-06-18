#include "SpeedController.hpp"

#include <algorithm>
#include <iostream>;
#include <limits>
#include <ranges>

namespace FastTransport::Protocol {
SpeedController::SpeedController()
    : _packetPerSecond(MinSpeed)
    , _up(true)
    , _speedIncrement(1)
{
}

size_t SpeedController::GetNumberPacketToSend()
{
    auto realSpeedStats = _stats | std::views::filter([](const SampleStats& sample) {
        return sample.lost < 5;
    });

    auto maxRealSpeedIterator = std::ranges::max_element(realSpeedStats, {}, &SampleStats::speed);
    int maxRealSpeed = 1;
    if (maxRealSpeedIterator != realSpeedStats.end()) {
        maxRealSpeed = maxRealSpeedIterator.base()->speed;
    }

    auto lostSpeedStats = _stats | std::views::filter([](const SampleStats& sample) {
        return sample.lost >= 5;
    });

    auto minLostSpeedIterator = std::ranges::min_element(lostSpeedStats, {}, &SampleStats::speed);
    int minLostSpeed = std::numeric_limits<int>::max();
    if (minLostSpeedIterator != lostSpeedStats.end()) {
        minLostSpeed = minLostSpeedIterator.base()->speed;
    }

    long long newSpeed = _packetPerSecond;

    if (minLostSpeed == std::numeric_limits<int>::max()) {
        if (_up) {
            _speedIncrement *= 2;
            newSpeed += _speedIncrement;
        } else {
            _up = true;
            _speedIncrement /= 2;
            _speedIncrement = std::max<int>(_speedIncrement, 1);
            newSpeed += _speedIncrement;
        }
    } else {
        if (minLostSpeed > newSpeed) {
            if (_up) {
                _speedIncrement *= 2;
                newSpeed += _speedIncrement;
            } else {
                _up = true;
                _speedIncrement /= 2;
                _speedIncrement = std::max<int>(_speedIncrement, 1);
                newSpeed += _speedIncrement;
            }
        } else {
            if (_up) {
                _up = false;
                _speedIncrement /= 2;
                _speedIncrement = std::max<int>(_speedIncrement, 1);
                newSpeed -= _speedIncrement;
            } else {
                _speedIncrement *= 2;
                newSpeed -= _speedIncrement;
            }
        }
    }

    _packetPerSecond = std::max<int>(newSpeed, MinSpeed);
    if (_packetPerSecond > maxRealSpeed * 3) {
        _speedIncrement /= 2;
        _speedIncrement = std::max<int>(_speedIncrement, 1);
        _packetPerSecond = maxRealSpeed * 10;
    }
    /*
    std::cout << "inc:" << _speedIncrement << std::endl;
    std::cout << "max:" << maxRealSpeed << std::endl;
    std::cout << "lost:" << minLostSpeed << std::endl;
    std::cout << "Speed:" << _packetPerSecond << std::endl;
    */
    return _packetPerSecond;
}

void SpeedController::UpdateStats(const SampleStats& stats)
{
    if (_stats.empty() || _stats.back().allPackets > SampleStats::MinPacketsCount) {
        _stats.push_back(stats);
    } else {
        _stats.back().Merge(stats);
    }
}
} // namespace FastTransport::Protocol