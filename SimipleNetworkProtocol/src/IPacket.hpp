#pragma once

#include "ConnectionAddr.hpp"
#include "HeaderBuffer.hpp"
#include "MultiList.hpp"

namespace FastTransport::Protocol {
class IPacket {

public:
    using Ptr = std::unique_ptr<IPacket>;
    using List = Containers::MultiList<Ptr>;
    using PairList = std::pair<List, List>;

    using ElementType = std::vector<unsigned char>;

    IPacket() = default;
    IPacket(const IPacket& that) = default;
    IPacket(IPacket&& that) = default;
    IPacket& operator=(const IPacket& that) = default;
    IPacket& operator=(IPacket&& that) = default;
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
} // namespace FastTransport::Protocol
