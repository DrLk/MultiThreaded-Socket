#pragma once

#include <chrono>
#include "BufferOwner.h"

namespace FastTransport
{
    namespace   Protocol
    {
        class OutgoingPacket : public FreeableBuffer
        {
        public:
            OutgoingPacket(std::shared_ptr<BufferOwner>& buffer) : FreeableBuffer(buffer) { }
            OutgoingPacket(std::shared_ptr<BufferOwner>&& buffer) noexcept : FreeableBuffer(std::move(buffer)) { }
        private:

            std::chrono::microseconds _time;
        };
    }
}
