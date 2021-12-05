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
            typedef std::chrono::steady_clock clock;

            std::unordered_map<SeqNumberType, OutgoingPacket> _queue;

        public:
            std::list<std::unique_ptr<IPacket>> AddQueue(std::list<OutgoingPacket>&& packets);
            std::list<std::unique_ptr<IPacket>> ProcessAcks(const SelectiveAckBuffer::Acks& acks);
            std::list<OutgoingPacket> CheckTimeouts();
        };
    }
}