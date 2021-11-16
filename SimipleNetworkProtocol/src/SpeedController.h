#pragma once

#include <chrono>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include "HeaderBuffer.h"
#include "BufferOwner.h"


namespace FastTransport
{
    namespace Protocol
    {
        class Sample
        {
        public:
            Sample(const std::chrono::nanoseconds & sendInterval, const std::chrono::microseconds& startTime) : _lostPackets(0),
                _start(0),
                _end(0),
                _sendInterval(sendInterval),
                _startTime(startTime)
            {
            }

            void ProcessAck(SeqNumberType number)
            {
                auto packet = _packets.find(number);
                if (packet != _packets.end())
                {
                    _lostPackets--;
                    _packets.erase(packet);
                }
            }

            void ProcessTimeoutPacket(SeqNumberType number)
            {
                auto packet = _packets.find(number);
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

            bool IsCompleted() const
            {
                return _packets.empty();
            }

            bool IsBetter(const Sample& that) const
            {

                size_t count =(size_t)( _end - _start);
                if (count < _packets.size() * 4)
                    return false;

                if (GetRealSpeed() > that.GetRealSpeed())
                    return true;
            }


            std::chrono::nanoseconds GetRealSpeed() const
            {
                int count =(int)( _end - _start);
                float ackPacketsPercent = (float)(count - _lostPackets) / (float)count;

                return std::chrono::nanoseconds((long long)((float)_sendInterval.count() / ackPacketsPercent));
            }

        private:
            std::unordered_map<SeqNumberType, BufferOwner::Ptr> _packets;

            int _lostPackets;
            SeqNumberType _start;
            SeqNumberType _end;
            std::chrono::nanoseconds _sendInterval;
            std::chrono::microseconds _startTime;

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

            void GetStableSendInterval()
            {
                std::max_element(_samples.begin(), _samples.end(), std::bind(&Sample::IsBetter, std::placeholders::_1, std::placeholders::_2));
                for (Sample& sample : _samples)
                {
                    sample.GetRealSpeed();
                }
            }
        };
    }
}
