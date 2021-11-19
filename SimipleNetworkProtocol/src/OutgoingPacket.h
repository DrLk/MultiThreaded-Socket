#pragma once

#include <chrono>

#include "IPacket.h"

namespace FastTransport
{
    namespace   Protocol
    {
        typedef std::chrono::steady_clock Time;
        typedef std::chrono::steady_clock::time_point TimePoint;
        typedef std::chrono::milliseconds ms;

        class OutgoingPacket
        {
        public:
            OutgoingPacket(std::unique_ptr<IPacket>&& packet) : _packet(std::move(packet)) { }
            std::unique_ptr<IPacket> _packet;
            TimePoint _sendTime;
        };
    }
}
