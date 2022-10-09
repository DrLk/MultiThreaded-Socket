#pragma once

#include <algorithm>
#include <limits>
#include <list>
#include <numeric>
#include <ranges>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

class SampleStats;
struct SpeedControllerState {
    int realSpeed;
};

class ISpeedControllerState {
public:
    virtual ISpeedControllerState* Run(const std::list<SampleStats>& stats, SpeedControllerState& state) = 0;

protected:
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;

    static int GetMaxRealSpeed(const std::list<SampleStats>& stats)
    {
        auto realSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.lost < 5;
        });

        auto maxRealSpeedIterator = std::ranges::max_element(realSpeedStats, {}, &SampleStats::speed);
        int maxRealSpeed = 1;
        if (maxRealSpeedIterator != realSpeedStats.end()) {
            maxRealSpeed = maxRealSpeedIterator.base()->speed;
        }

        return maxRealSpeed;
    }

    static int GetMinLostSpeed(const std::list<SampleStats>& stats)
    {
        auto lostSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.lost >= 5;
        });

        auto minLostSpeedIterator = std::ranges::min_element(lostSpeedStats, {}, &SampleStats::speed);
        int minLostSpeed = (std::numeric_limits<int>::max)();
        if (minLostSpeedIterator != lostSpeedStats.end()) {
            minLostSpeed = minLostSpeedIterator.base()->speed;
        }

        return minLostSpeed;
    }
};

class FastAccelerationState : public ISpeedControllerState {
public:
    FastAccelerationState()
         
    = default;

    ISpeedControllerState* Run(const std::list<SampleStats>& stats, SpeedControllerState& state) override
    {
        int newSpeed = state.realSpeed;

        int minLostSpeed = ISpeedControllerState::GetMinLostSpeed(stats);
        int maxRealSpeed = ISpeedControllerState::GetMaxRealSpeed(stats);

        if (minLostSpeed == (std::numeric_limits<int>::max)()) {
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

        if (state.realSpeed > maxRealSpeed * 3) {
            _speedIncrement /= 2;
            _speedIncrement = std::max<int>(_speedIncrement, 1);
            state.realSpeed = maxRealSpeed * 10;
        }

        state.realSpeed = std::max<int>(newSpeed, MinSpeed);

        return this;
    }

private:
    int _speedIncrement{1};
    bool _up{true};
};
} // namespace FastTransport::Protocol