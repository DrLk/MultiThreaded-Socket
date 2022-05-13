#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"
#include "Sample.hpp"

namespace FastTransport::Protocol {
class SpeedController {
    using clock = std::chrono::steady_clock;

public:
    SpeedController()
        : _lastSend(clock::now())

    {
    }

    size_t GetNumberPacketToSend()
    {
        auto now = clock::now();
        auto duration = now - _lastSend;
        _lastSend = now;

        size_t numberPeracketToSend = (duration / std::chrono::milliseconds(1)) * _packetPerMilisecond;

        return numberPeracketToSend;
    }

private:
    clock::time_point _lastSend;
    size_t _packetPerMilisecond { 10 };
    static constexpr size_t MinSpeed = 10;
    static constexpr size_t MaxSpeed = 10000;

    void GetStableSendInterval()
    {
        // std::max_element(_samples.begin(), _samples.end(), std::bind(&Sample::IsBetter, std::placeholders::_1, std::placeholders::_2));
        /*for (Sample& sample : _samples)
        {
            sample.GetRealSpeed();
        }*/
    }
};
} // namespace FastTransport::Protocol
