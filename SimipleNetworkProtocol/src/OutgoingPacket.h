#pragma once

#include <chrono>

#include "IPacket.h"

namespace FastTransport
{
    namespace   Protocol
    {
        class OutgoingPacket
        {
            typedef std::chrono::steady_clock clock;

        public:
            OutgoingPacket() = default;
            OutgoingPacket(IPacket::Ptr&& packet, bool needAck) : _packet(std::move(packet)), _needAck(needAck)
            {
            }

            IPacket::Ptr _packet;
            clock::time_point _sendTime;
            bool _needAck;
        };
    }
}
