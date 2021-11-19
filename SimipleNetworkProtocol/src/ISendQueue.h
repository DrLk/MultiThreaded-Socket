#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>

#include "IPacket.h"
#include "OutgoingPacket.h"

namespace FastTransport
{
    namespace Protocol
    {

        class ISendQueue
        {
        public:
            virtual ~ISendQueue() { }
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
