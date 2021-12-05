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
            OutgoingPacket(std::unique_ptr<IPacket>&& packet) : _packet(std::move(packet))
            {
            }

            std::unique_ptr<IPacket> _packet;
            clock::time_point _sendTime;
        };
    }
}
