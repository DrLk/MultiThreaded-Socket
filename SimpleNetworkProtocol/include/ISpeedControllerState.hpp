#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <functional>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

#include "TimeRangedStats.hpp"

namespace FastTransport::Protocol {

using namespace std::chrono_literals;

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

    virtual ISpeedControllerState* Run(const TimeRangedStats& stats, SpeedControllerState& state) = 0;

protected:
    static constexpr size_t MinSpeed = 100;
    static constexpr size_t MaxSpeed = 10000;

    static constexpr float MaxPacketLost = 5.0;

    static auto GetSlides(const std::vector<SampleStats>& stats, int width)
    {
        std::vector<std::ranges::take_view<std::ranges::drop_view<std::ranges::ref_view<const std::vector<FastTransport::Protocol::SampleStats>>>>> slides;
        slides.reserve(stats.size() - width);
        for (int i = 0; i < stats.size() - width; i++) {
            slides.push_back(std::views::drop(stats, i) | std::views::take(width));
        }

        return slides;
    }

    template <class Range>
    static auto GetStatsPer(const Range& stats, int width)
    {
        const auto& slides = GetSlides(stats, width); // local variables: problem

        auto result = slides | std::views::transform([](const auto& samples) {
            SampleStats stat = std::accumulate(samples.begin(), samples.end(), SampleStats(), [](SampleStats&& stat, const SampleStats& sample) {
                stat.Merge(sample);
                return stat;
            });

            return stat;
        });

        return std::vector<SampleStats>(result.begin(), result.end());
    }

    static std::optional<int> GetMaxRealSpeed(const std::vector<SampleStats>& stats)
    {
        auto realSpeedStats = stats | std::views::filter([](const SampleStats& sample) {
            return sample.GetLost() < MaxPacketLost;
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
            return sample.GetLost() >= MaxPacketLost;
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

        result = std::accumulate(stats.begin(), stats.end(), 0, [](int accumulator, const SampleStats& sample) {
            return accumulator + sample.GetLostPackets();
        });

        return result;
    }
};

class BBQState final : public ISpeedControllerState {
public:
    BBQState() = default;

    using clock = std::chrono::steady_clock;
    ISpeedControllerState* Run(const TimeRangedStats& stats, SpeedControllerState& state) override
    {
        int statsWidth = stats.GetMaxRtt() / TimeRangedStats::Interval + 1;
        auto statsBySpeed = GroupByAllPackets(GetStatsPer(stats.GetSamplesStats(), statsWidth));

        auto maxRealSpeedIterator = std::ranges::max_element(statsBySpeed, [](const auto& left, const auto& right) {
            return GetMetric(left.second) < GetMetric(right.second);
        });

        if (maxRealSpeedIterator == statsBySpeed.end()) {
            state.realSpeed = ISpeedControllerState::MinSpeed;
            return this;
        }

        const int totalPackets = std::accumulate(maxRealSpeedIterator->second.begin(), maxRealSpeedIterator->second.end(), 0,
            [](int accumulator, const SampleStats& stat) {
                accumulator += stat.GetAllPackets() - stat.GetLostPackets();
                return accumulator;
            });

        if (totalPackets != 0) {
            state.realSpeed = totalPackets / static_cast<int>(maxRealSpeedIterator->second.size());
            state.realSpeed = static_cast<int>(state.realSpeed * SpeedIncreaseRatio / statsWidth);
        } else {
            state.realSpeed = ISpeedControllerState::MinSpeed;
        }

        return this;
    }

private:
    static constexpr double SpeedIncreaseRatio = 1.4;

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

    template <class Range>
    static std::map<ApproximateInt, std::vector<SampleStats>> GroupByAllPackets(const Range& sampleStats)
    {
        std::map<ApproximateInt, std::vector<SampleStats>> result;

        for (const auto& sampleStat : sampleStats) {
            auto stat = result.find(ApproximateInt(sampleStat.GetAllPackets()));

            if (sampleStat.GetLost() > MaxPacketLost) {
                continue;
            }

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

class FastAccelerationState final : public ISpeedControllerState {
public:
    FastAccelerationState() = default;

    ISpeedControllerState* Run(const TimeRangedStats& stats, SpeedControllerState& state) override
    {
        int newSpeed = state.realSpeed;

        std::optional<int> const minLostSpeed = ISpeedControllerState::GetMinLostSpeed(stats.GetSamplesStats());
        std::optional<int> const maxRealSpeed = ISpeedControllerState::GetMaxRealSpeed(stats.GetSamplesStats());

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

class StableState final : public ISpeedControllerState {
public:
    StableState() = default;

    ISpeedControllerState* Run(const TimeRangedStats& /*stats*/, SpeedControllerState& state) override
    {
        // state.realSpeed = MinSpeed;
        state.realSpeed = 10000;

        return this;
    }
};

} // namespace FastTransport::Protocol