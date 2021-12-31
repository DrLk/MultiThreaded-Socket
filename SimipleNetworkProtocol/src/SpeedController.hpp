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
public:
    SpeedController()
        : _stableSendInterval(0)
    {
    }

    std::chrono::nanoseconds GetSendInterval()
    {
        return _stableSendInterval;
    }

private:
    std::chrono::nanoseconds _stableSendInterval;
    std::vector<Sample> _samples;
    static const int MinSpeed = 10;
    static const int MaxSpeed = 10000;

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
