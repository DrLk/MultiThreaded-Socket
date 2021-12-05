#pragma once

#include <unordered_map>
#include <vector>
#include <chrono>

#include "IPacket.h"
#include "OutgoingPacket.h"
#include "LockedList.h"

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
                {
                    std::lock_guard<std::mutex> lock(_needToSend._mutex);
                    _needToSend.emplace_back(std::move(packet), needAck);
                }
            }

            void ReSendPackets(std::list<OutgoingPacket>&& packets)
            {
                std::lock_guard<std::mutex> lock(_needToSend._mutex);
                _needToSend.splice(_needToSend.end(), std::move(packets));
            }

            //make list of list to get fast 1k packets
            std::list<OutgoingPacket> GetPacketsToSend()
            {
                std::list<OutgoingPacket> result;
                {
                    std::lock_guard<std::mutex> lock(_needToSend._mutex);
                    result = std::move(_needToSend);
                }

                return result;
            }

            LockedList<OutgoingPacket> _needToSend;
            SeqNumberType _nextPacketNumber;

        };
    }
}
