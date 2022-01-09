#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

#include "HeaderBuffer.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {
class Sample {
public:
    Sample(const std::chrono::microseconds& sendInterval, const std::chrono::microseconds& startTime)
        : _lostPackets(0)
        , _start(0)
        , _end(0)
        , _sendInterval(sendInterval)
        , _startTime(startTime)

    {
    }

private:
    std::unordered_map<SeqNumberType, IPacket::Ptr> _packets;

    int _lostPackets;
    SeqNumberType _start;
    SeqNumberType _end;
    std::chrono::microseconds _sendInterval;
    std::chrono::microseconds _startTime;
};

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

        size_t numberPeracketToSend = (duration / std::chrono::milliseconds(1)) * packetPerMilisecond;

        return numberPeracketToSend;
    }

private:
    clock::time_point _lastSend;
    size_t packetPerMilisecond { 1 };
    std::vector<Sample> _samples;
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
