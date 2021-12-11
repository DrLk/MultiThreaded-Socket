#pragma once

#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "OutgoingPacket.h"


namespace FastTransport
{
    namespace Protocol
    {
        class IInflightQueue
        {
            typedef std::chrono::steady_clock clock;

            std::mutex _queueMutex;
            std::unordered_map<SeqNumberType, OutgoingPacket> _queue;
            std::mutex _receivedAcksMutex;
            std::unordered_set<SeqNumberType> _receivedAcks;

        public:
            std::list<std::unique_ptr<IPacket>> AddQueue(std::list<OutgoingPacket>&& packets);
            std::list<std::unique_ptr<IPacket>> ProcessAcks(const SelectiveAckBuffer::Acks& acks);
            std::list<OutgoingPacket> CheckTimeouts();
        };
    }
}