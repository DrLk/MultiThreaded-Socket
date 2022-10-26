#pragma once

#include <algorithm>
#include <ranges>
#include <vector>
#include <optional>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

class SampleStats;
struct SpeedControllerState {
    int realSpeed;
};

class ISpeedControllerState {
public:
    virtual ISpeedControllerState* Run(const std::vector<SampleStats>& stats, SpeedControllerState& state) = 0;

protected:
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;

    static std::optional<long long> GetMaxRealSpeed(const std::vector<SampleStats>& stats)
    {
        auto realSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.lost < 5;
        });

        auto maxRealSpeedIterator = std::ranges::max_element(realSpeedStats, {}, &SampleStats::speed);
        std::optional<long long> maxRealSpeed;
        if (maxRealSpeedIterator != realSpeedStats.end()) {
            maxRealSpeed = maxRealSpeedIterator.base()->speed;
        }

        return maxRealSpeed;
    }

    static std::optional<long long> GetMinLostSpeed(const std::vector<SampleStats>& stats)
    {
        auto lostSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.lost >= 5;
        });

        auto minLostSpeedIterator = std::ranges::min_element(lostSpeedStats, {}, &SampleStats::speed);

        std::optional<long long> minLostSpeed;
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

    ISpeedControllerState* Run(const std::vector<SampleStats>& stats, SpeedControllerState& state) override
    {
        int newSpeed = state.realSpeed;

        std::optional<long long> minLostSpeed = ISpeedControllerState::GetMinLostSpeed(stats);
        std::optional<long long> maxRealSpeed = ISpeedControllerState::GetMaxRealSpeed(stats);

        if (!minLostSpeed.has_value()) {
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

        if (state.realSpeed > maxRealSpeed.value_or(0) * 3) {
            _speedIncrement /= 2;
            _speedIncrement = std::max<int>(_speedIncrement, 1);
            state.realSpeed = maxRealSpeed.value_or(0) * 10;
        }

        state.realSpeed = std::max<int>(newSpeed, MinSpeed);

        return this;
    }

private:
    int _speedIncrement { 1 };
    bool _up { true };
};
} // namespace FastTransport::Protocol