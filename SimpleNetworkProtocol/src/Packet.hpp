#pragma once

#include <chrono>

#include "IPacket.hpp"
#include "LockedList.hpp"

namespace FastTransport::Protocol {

class Packet final : public IPacket {
public:
    using Ptr = std::shared_ptr<Packet>;

    Packet(const Packet& that) = delete;
    Packet& operator=(const Packet& that) = delete;
    Packet(Packet&& that) = delete;
    Packet& operator=(Packet&& that) = delete;

    explicit Packet(int size);
    ~Packet() override = default;

    [[nodiscard]] std::span<SeqNumberType> GetAcks() override;
    void SetAcks(const std::list<SeqNumberType>& acks) override;

    [[nodiscard]] PacketType GetPacketType() const override;
    void SetPacketType(PacketType type) override;
    [[nodiscard]] ConnectionID GetSrcConnectionID() const override;
    void SetSrcConnectionID(ConnectionID connectionId) override;
    [[nodiscard]] ConnectionID GetDstConnectionID() const override;
    void SetDstConnectionID(ConnectionID connectionId) override;
    [[nodiscard]] SeqNumberType GetSeqNumber() const override;
    void SetSeqNumber(SeqNumberType seq) override;
    [[nodiscard]] SeqNumberType GetAckNumber() const override;
    void SetAckNumber(SeqNumberType ack) override;
    void SetMagic() override;
    [[nodiscard]] bool IsValid() const override;

    std::span<ElementType> GetPayload() override
    {
        throw std::runtime_error("Not implemented");
    }

    [[nodiscard]] const ConnectionAddr& GetDstAddr() const override
    {
        return _dstAddr;
    }

    // TODO: remove and use GetDstAddr for assign
    void SetAddr(const ConnectionAddr& addr) override
    {
        _dstAddr = addr;
    }

    [[nodiscard]] std::chrono::nanoseconds GetTime() const
    {
        return _time;
    }

    void SetTime(const std::chrono::nanoseconds& time)
    {
        _time = time;
    }

    std::vector<ElementType>& GetElement() override
    {
        return _element;
    }

    void Copy(const IPacket& packet) override
    {
        const auto& that = dynamic_cast<const Packet&>(packet);

        _srcAddr = that._srcAddr;
        _dstAddr = that._dstAddr;
        _time = that._time;
        _element = that._element;
    }

private:
    mutable std::vector<ElementType> _element;

    ConnectionAddr _srcAddr;
    ConnectionAddr _dstAddr;
    std::chrono::nanoseconds _time;
};

} // namespace FastTransport::Protocol