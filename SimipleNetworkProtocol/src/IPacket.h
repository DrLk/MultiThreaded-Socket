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
            virtual SelectiveAckBuffer GetAcksBuffer() = 0;

            virtual HeaderBuffer GetHeaderBuffer() = 0;

            virtual SelectiveAckBuffer::Acks GetAcks() = 0;

            virtual HeaderBuffer::Header GetHeader() = 0;

            virtual ConnectionAddr GetDstAddr()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual PayloadBuffer::Payload GetPayload()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual void Copy(const IPacket& packet) = 0;
        };
    }
}
