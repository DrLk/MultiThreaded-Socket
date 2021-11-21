#pragma once
#include "HeaderBuffer.h"
#include "ConnectionAddr.h"

namespace FastTransport
{
    namespace Protocol
    {
        class IPacket
        {

        public:
            virtual ~IPacket() = default;

            virtual SelectiveAckBuffer GetAcksBuffer() = 0;

            virtual SelectiveAckBuffer::Acks GetAcks() = 0;

            virtual Header GetHeader() = 0;

            virtual ConnectionAddr GetDstAddr() = 0;

            virtual PayloadBuffer::Payload GetPayload() = 0;

            virtual void Copy(const IPacket& packet) = 0;
        };
    }
}
