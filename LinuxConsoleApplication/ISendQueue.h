#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>

#include "BufferOwner.h"

namespace FastTransport
{
    namespace Protocol
    {
        typedef std::chrono::steady_clock Time;
        typedef std::chrono::steady_clock::time_point TimePoint;
        typedef std::chrono::milliseconds ms;
        class OutgoingPacket
        {
            OutgoingPacket(BufferOwner::Ptr& packet) : _packet(packet), _sendTime(0)
        public:
            BufferOwner::Ptr _packet;
            TimePoint _sendTime;
        };
        class ISendQueue
        {
        public:
            virtual ~ISendQueue() { }
            virtual void ProcessAcks(const SelectiveAckBuffer::Acks& acks) = 0;
        };

        class SendQueue : public ISendQueue
        {
            typedef std::vector<char> DataType;

        public:

            void SendPacket(DataType data)
            {

            }

            void SendPacket(BufferOwner::Ptr& packet)
            {
                packet.GetHeader().SetSeqNumber(++_nextPacketNumber);
                _inFlightPackets.insert({_nextPacketNumber, packet});
                _needToSend.push_back(packet);
            }

            virtual void ProcessAcks(const SelectiveAckBuffer::Acks& acks) override
            {
                for (SeqNumberType number : acks)
                {
                    _inFlightPackets.erase(number);
                }
            }

            void CheckTimeouts()
            {
                TimePoint now = Time::now();
                for (auto pair : _inFlightPackets)
                {
                    OutgoingPacket packet = pair.second;
                    if (std::chrono::duration_cast<ms>(now - packet._sendTime) > ms(10))
                    {
                        _needToSend.push_back(packet);
                    }
                }
            }

            std::vector<OutgoingPacket>& GetPacketsToSend()
            {
                return _needToSend;
            }

            std::unordered_map<SeqNumberType, OutgoingPacket> _inFlightPackets;
            std::vector<OutgoingPacket> _needToSend;
            SeqNumberType _nextPacketNumber;

        };
    }
}
