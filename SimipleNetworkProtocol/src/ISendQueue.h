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
                packet->GetHeader().SetMagic();
                packet->GetHeader().SetSeqNumber(++_nextPacketNumber);
                _needToSend.push_back(std::move(packet));
            }

            void ReSendPackets(std::list<OutgoingPacket>&& packets)
            {
                _needToSend.splice(_needToSend.end(), std::move(packets));
            }

            std::list<OutgoingPacket>& GetPacketsToSend()
            {
                return _needToSend;
            }

            std::list<OutgoingPacket> _needToSend;
            SeqNumberType _nextPacketNumber;

        };
    }
}
