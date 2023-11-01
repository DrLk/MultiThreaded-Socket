#pragma once

#include <memory> // IWYU pragma: export
#include <span>
#include <vector>

#include "HeaderTypes.hpp"
#include "MultiList.hpp" // IWYU pragma: export

namespace FastTransport::Protocol {
class ConnectionAddr;
} // namespace FastTransport::Protocol

namespace FastTransport::Protocol {
class IPacket {

public:
    using Ptr = std::unique_ptr<IPacket>;
    using List = Containers::MultiList<Ptr>;

    using ElementType = unsigned char;

    IPacket() = default;
    IPacket(const IPacket& that) = default;
    IPacket(IPacket&& that) noexcept = default;
    IPacket& operator=(const IPacket& that) = default;
    IPacket& operator=(IPacket&& that) noexcept = default;
    virtual ~IPacket() = default;

    [[nodiscard]] virtual std::span<SeqNumberType> GetAcks() = 0;
    virtual void SetAcks(std::span<const SeqNumberType> acks) = 0;
    [[nodiscard]] virtual PacketType GetPacketType() const = 0;
    virtual void SetPacketType(PacketType type) = 0;
    [[nodiscard]] virtual ConnectionID GetSrcConnectionID() const = 0;
    virtual void SetSrcConnectionID(ConnectionID connectionId) = 0;
    [[nodiscard]] virtual ConnectionID GetDstConnectionID() const = 0;
    virtual void SetDstConnectionID(ConnectionID connectionId) = 0;
    [[nodiscard]] virtual SeqNumberType GetSeqNumber() const = 0;
    virtual void SetSeqNumber(SeqNumberType seq) = 0;
    [[nodiscard]] virtual SeqNumberType GetAckNumber() const = 0;
    virtual void SetAckNumber(SeqNumberType ack) = 0;
    virtual void SetMagic() = 0;
    [[nodiscard]] virtual bool IsValid() const = 0;

    [[nodiscard]] virtual const ConnectionAddr& GetDstAddr() const = 0;
    virtual void SetAddr(const ConnectionAddr& addr) = 0;

    virtual std::span<ElementType> GetPayload() = 0;
    virtual void SetPayload(std::span<ElementType> payload) = 0;

    virtual void Copy(const IPacket& packet) = 0;

    virtual std::vector<ElementType>& GetElement() = 0;
    virtual std::span<ElementType> GetBuffer() = 0;
};
} // namespace FastTransport::Protocol
