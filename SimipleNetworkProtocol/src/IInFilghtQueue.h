#pragma once

#include <unordered_map>
#include <memory>

#include "OutgoingPacket.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IInflightQueue
        {
            std::unordered_map<SeqNumberType, OutgoingPacket> _queue;

        public:
            void AddQueue(std::unique_ptr<OutgoingPacket>&& packet);
            void AddQueue(std::list<OutgoingPacket>&& packets);
            void ProcessAcks(const SelectiveAckBuffer::Acks& acks);
            std::list<OutgoingPacket> CheckTimeouts();
        };
    }
}