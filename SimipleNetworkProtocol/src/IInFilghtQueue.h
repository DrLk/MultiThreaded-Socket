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
            void AddQueue(std::unique_ptr<IPacket>&& packet);
            void AddQueue(std::list<std::unique_ptr<IPacket>>&& packets);
            std::list<std::unique_ptr<IPacket>> ProcessAcks(const SelectiveAckBuffer::Acks& acks);
        };
    }
}