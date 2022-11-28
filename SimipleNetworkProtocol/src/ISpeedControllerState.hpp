#pragma once

#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <vector>

#include "SampleStats.hpp"

namespace FastTransport::Protocol {

class SampleStats;
struct SpeedControllerState {
    int realSpeed;
};

class ISpeedControllerState {
public:
    ISpeedControllerState() = default;
    ISpeedControllerState(const ISpeedControllerState&) = default;
    ISpeedControllerState(ISpeedControllerState&&) = default;
    ISpeedControllerState& operator=(const ISpeedControllerState&) = default;
    ISpeedControllerState& operator=(ISpeedControllerState&&) = default;
    virtual ~ISpeedControllerState() = default;

    virtual ISpeedControllerState* Run(const std::vector<SampleStats>& stats, SpeedControllerState& state) = 0;

protected:
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;

    static std::optional<int> GetMaxRealSpeed(const std::vector<SampleStats>& stats)
    {
        auto realSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.GetLost() < 5;
        });

        auto maxRealSpeedIterator = std::ranges::max_element(realSpeedStats,
            [](const SampleStats& left, const SampleStats& right) {
                return left.GetAllPackets() < right.GetAllPackets();
            });

        std::optional<int> maxRealSpeed;
        if (maxRealSpeedIterator != realSpeedStats.end()) {
            maxRealSpeed = maxRealSpeedIterator.base()->GetAllPackets();
        }

        return maxRealSpeed;
    }

    static std::optional<int> GetMinLostSpeed(const std::vector<SampleStats>& stats)
    {
        auto lostSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.GetLost() >= 5;
        });

        auto minLostSpeedIterator = std::ranges::min_element(lostSpeedStats,
            [](const SampleStats& left, const SampleStats& right) {
                return left.GetAllPackets() < right.GetAllPackets();
            });

        std::optional<int> minLostSpeed;
        if (minLostSpeedIterator != lostSpeedStats.end()) {
            minLostSpeed = minLostSpeedIterator.base()->GetAllPackets();
        }

        return minLostSpeed;
    }

    static std::optional<int> GetAllPackets(const std::vector<SampleStats>& stats)
    {
        std::optional<int> result;

        if (stats.empty()) {
            return result;
        }

        result = std::accumulate(stats.begin(), stats.end(), 0, [](int accumulator, const SampleStats& sample) {
            return accumulator + sample.GetAllPackets();
        });

        return result;
    }

    static std::optional<int> GetLostPackets(const std::vector<SampleStats>& stats)
    {
        std::optional<int> result;

        if (stats.empty()) {
            return result;
        }

        result = std::accumulate(stats.begin(), stats.end(), 0, [](int accumulator, SampleStats sample) {
            return accumulator + sample.GetLostPackets();
        });

        return result;
    }
};

class BBQState : public ISpeedControllerState {
public:
    BBQState() = default;

    ISpeedControllerState* Run(const std::vector<SampleStats>& stats, SpeedControllerState& state) override
    {
        auto statsBySpeed = GroupByAllPackets(stats);

        auto maxRealSpeedIterator = std::ranges::max_element(statsBySpeed, [](const auto& left, const auto& right) {
            return GetMetric(left.second) < GetMetric(right.second);
        });

        if (maxRealSpeedIterator == statsBySpeed.end()) {
            state.realSpeed = ISpeedControllerState::MinSpeed;
            return this;
        }

        const int totalPackets = std::accumulate(maxRealSpeedIterator->second.begin(), maxRealSpeedIterator->second.end(), 0,
            [](int accumulator, const SampleStats& stat) {
                accumulator += stat.GetAllPackets();
                return accumulator;
            });

        if (totalPackets != 0) {
            state.realSpeed = totalPackets / static_cast<int>(maxRealSpeedIterator->second.size());
            state.realSpeed = static_cast<int>(state.realSpeed * SpeedIncreaseRatio);
        } else {
            state.realSpeed = ISpeedControllerState::MinSpeed;
        }

        return this;
    }

private:
    static constexpr double SpeedIncreaseRatio = 1.2;

    class ApproximateInt {
    public:
        explicit ApproximateInt(int number)
            : _number(number)
        {
        }

        bool operator<(const ApproximateInt& other) const // NOLINT(fuchsia-overloaded-operator)
        {
            return _number * Rounding < other._number;
        }

    private:
        int _number;
        static constexpr double Rounding = 1.1;
    };

    static std::map<ApproximateInt, std::vector<SampleStats>> GroupByAllPackets(const std::vector<SampleStats>& sampleStats)
    {
        std::map<ApproximateInt, std::vector<SampleStats>> result;

        for (const auto& sampleStat : sampleStats) {
            auto stat = result.find(ApproximateInt(sampleStat.GetAllPackets()));

            if (stat == result.end()) {
                result[ApproximateInt(sampleStat.GetAllPackets())].push_back(sampleStat);
            } else {
                stat->second.push_back(sampleStat);
            }
        }

        return result;
    }

    static double GetMetric(const std::vector<SampleStats>& samples)
    {

        static constexpr double LostRatio = 1.1;

        std::optional<int> allPackets = ISpeedControllerState::GetAllPackets(samples);
        std::optional<int> lostPackets = ISpeedControllerState::GetLostPackets(samples);

        if (!allPackets.has_value()) {
            return 0;
        }

        if (!lostPackets.has_value()) {
            return 0;
        }

        return allPackets.value() - lostPackets.value() * LostRatio;
    }
};

class FastAccelerationState : public ISpeedControllerState {
public:
    FastAccelerationState() = default;

    ISpeedControllerState* Run(const std::vector<SampleStats>& stats, SpeedControllerState& state) override
    {
        int newSpeed = state.realSpeed;

        std::optional<int> const minLostSpeed = ISpeedControllerState::GetMinLostSpeed(stats);
        std::optional<int> const maxRealSpeed = ISpeedControllerState::GetMaxRealSpeed(stats);

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