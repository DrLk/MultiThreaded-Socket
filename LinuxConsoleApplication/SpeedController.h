#pragma once

#include <chrono>
#include <vector>
#include <unordered_map>

#include "BufferOwner.h"


class Sample
{
    public:
        Sample(const std::chrono:nanoseconds sendInterval, const std::chrono::microseconds startTime) : _lostPackets(0),
                                                                                                        _start(0),
                                                                                                        _end(0),
                                                                                                        _sendInterval(sendInterval),
                                                                                                        _startTIme(startTime)
        {
        }

        void ProcessAck(SeqNumberType number)
        {
            auto& packet = _packets.find(number);
            if (packet != _packets.end())
            {
                _lostPackets--;
                _packets.erase(packet);
            }
        }

        void ProcessTimeoutPacket(SeqNumberType number)
        {
            auto& packet = _packets.find(number);
            if (packet != _packets.end())
            {
                _packets.erase(packet);
            }
        }

        bool Contains(SeqNumberType number) const
        {
            if (number > _end)
                return false;

            if (number < _start)
                return false;

            return true;
        }

        void AddPacket(const BufferOwner::Ptr& packet)
        {
            SeqNumberType number = packet->GetHeader().GetSeqNumber();
            if (_packets.empty())
            {
                _start = number;
                _end = number;
            }

            _packets.insert({ number, packet });
            _lostPackets++;

            packet->SetTime(_sendInterval);

            if (_start > number)
            {
                _start = number;
            }

            if (_end < number)
            {
                _end = number;
            }
        }


        std::chrono::microseconds GetStartTime()
        {
            return _startTime;
        }

    private:
        std::unordered_map<SeqNumberType, BufferOwner::Ptr> _packets;

        int _lostPackets;
        SeqNumberType _start;
        SeqNumberType _end;
        std::chrono::microseconds _startTime;
        std::chrono::nanoseconds _sendInterval;

};

class SpeedController
{
    public:
        SpeedController()
        {
        }

        void ProcessAcks(const SelectiveAckBuffer::Acks& acks, std::chrono::microseconds receivedTime)
        {
            for (SeqNumberType number : acks.GetAcks())
            {
                for (Sample& sample : _samples)
                {
                    if (sample.Contains(number))
                        sample.ProcessAck(number);
                }
            }
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
};
