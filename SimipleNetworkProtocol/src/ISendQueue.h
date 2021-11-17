#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>

#include "IPacket.h"
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
        public:
            OutgoingPacket(std::unique_ptr<IPacket>&& packet) : _packet(std::move(packet))  { }
            std::unique_ptr<IPacket> _packet;
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

            SendQueue() : _nextPacketNumber(-1) { }

            void SendPacket(DataType data)
            {

            }

            void SendPacket(std::unique_ptr<IPacket>&& packet, bool needAck = false)
            {
                packet->GetSynAckHeader().SetMagic();
                packet->GetHeader().SetSeqNumber(++_nextPacketNumber);
                _needToSend.push_back(std::move(packet));
            }

            virtual void ProcessAcks(const SelectiveAckBuffer::Acks& acks) override
            {
                if (!acks.IsValid())
                    return;

                for (SeqNumberType number : acks.GetAcks())
                {
                    _inFlightPackets.erase(number);
                }
            }

            void CheckTimeouts()
            {
                TimePoint now = Time::now();
                for (auto& pair : _inFlightPackets)
                {
                    OutgoingPacket&& packet = std::move(pair.second);
                    if (std::chrono::duration_cast<ms>(now - packet._sendTime) > ms(10))
                    {
                        _needToSend.push_back(std::move(packet));
                    }
                }
            }

            std::list<OutgoingPacket>& GetPacketsToSend()
            {
                return _needToSend;
            }

            std::unordered_map<SeqNumberType, OutgoingPacket> _inFlightPackets;
            std::list<OutgoingPacket> _needToSend;
            SeqNumberType _nextPacketNumber;

        };
    }
}
