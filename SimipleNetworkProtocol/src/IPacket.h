#pragma once
#include "HeaderBuffer.h"
#include "ConnectionAddr.h"
#include "MultiList.h"

namespace FastTransport
{
    namespace Protocol
    {
        class IPacket
        {

        public:
            typedef std::unique_ptr<IPacket> Ptr;
            typedef Containers::MultiList<Ptr> List;
            typedef std::pair<List, List> PairList;

            typedef std::vector<char> ElementType;

            virtual ~IPacket() = default;

            virtual SelectiveAckBuffer GetAcksBuffer() = 0;

            virtual SelectiveAckBuffer::Acks GetAcks() = 0;

            virtual Header GetHeader() = 0;

            virtual ConnectionAddr& GetDstAddr() = 0;

            virtual void SetAddr(const ConnectionAddr& addr) = 0;

            virtual PayloadBuffer::Payload GetPayload() = 0;

            virtual void Copy(const IPacket& packet) = 0;

            virtual ElementType& GetElement() = 0;

        };
    }
}
