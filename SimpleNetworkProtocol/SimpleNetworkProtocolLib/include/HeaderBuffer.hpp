#pragma once

#include "cstddef"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>

#include "HeaderTypes.hpp"

namespace FastTransport::Protocol {

const MagicNumber Magic_Number = 0x12345678;
class Header : protected std::span<std::byte> {
public:
    Header(std::byte* start, size_t size)
        : std::span<std::byte>(start, size)
    {
    }

    [[nodiscard]] bool IsValid() const
    {
        if (size() < HeaderSize) {
            return false;
        }
        MagicNumber magic {};
        std::memcpy(&magic, data(), sizeof(magic));
        return magic == Magic_Number;
    }

    [[nodiscard]] PacketType GetPacketType() const
    {
        PacketType result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber)).data(), sizeof(result));
        return result;
    }

    [[nodiscard]] ConnectionID GetSrcConnectionID() const
    {
        ConnectionID result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber) + sizeof(PacketType)).data(), sizeof(result));
        return result;
    }

    [[nodiscard]] ConnectionID GetDstConnectionID() const
    {
        ConnectionID result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID)).data(), sizeof(result));
        return result;
    }

    [[nodiscard]] SeqNumberType GetSeqNumber() const
    {
        SeqNumberType result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID)).data(), sizeof(result));
        return result;
    }

    [[nodiscard]] SeqNumberType GetAckNumber() const
    {
        SeqNumberType result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType)).data(), sizeof(result));
        return result;
    }

    [[nodiscard]] PayloadSizeType GetPayloadSize() const
    {
        PayloadSizeType result {};
        std::memcpy(&result, subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType)).data(), sizeof(result));
        return result;
    }

    void SetMagic()
    {
        std::memcpy(data(), &Magic_Number, sizeof(Magic_Number));
    }

    void SetPacketType(PacketType type)
    {
        std::memcpy(subspan(sizeof(MagicNumber)).data(), &type, sizeof(type));
    }

    void SetSrcConnectionID(ConnectionID connectionId)
    {
        std::memcpy(subspan(sizeof(MagicNumber) + sizeof(PacketType)).data(), &connectionId, sizeof(connectionId));
    }

    void SetDstConnectionID(ConnectionID connectionId)
    {
        std::memcpy(subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID)).data(), &connectionId, sizeof(connectionId));
    }

    void SetSeqNumber(SeqNumberType seq)
    {
        std::memcpy(subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID)).data(), &seq, sizeof(seq));
    }

    void SetAckNumber(SeqNumberType ack)
    {
        std::memcpy(subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType)).data(), &ack, sizeof(ack));
    }

    void SetPayloadSize(PayloadSizeType payloadSize)
    {
        std::memcpy(subspan(sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType)).data(), &payloadSize, sizeof(payloadSize));
    }
};

class Acks {
public:
    Acks(std::byte* start, std::uint16_t size)
        : _start(start)
        , _size(size)
    {
    }

    [[nodiscard]] std::span<SeqNumberType> GetAcks() const
    {
        return { reinterpret_cast<SeqNumberType*>(std::next(_start, HeaderSize)), Header(_start, _size).GetPayloadSize() / sizeof(SeqNumberType) }; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    void SetAcks(std::span<const SeqNumberType> acks)
    {
        Header(_start, _size).SetPayloadSize(acks.size() * sizeof(SeqNumberType));
        std::ranges::copy(acks, reinterpret_cast<SeqNumberType*>(std::next(_start, HeaderSize))); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    [[nodiscard]] bool IsValid() const
    {
        return _size > 0;
    }

private:
    std::byte* _start;
    PayloadSizeType _size;
};

class Payload {
public:
    Payload(std::byte* start, size_t size)
        : _start(start)
        , _size(size)

    {
    }

    [[nodiscard]] std::span<PayloadType> GetPayload() const
    {
        return { reinterpret_cast<PayloadType*>(std::next(_start, HeaderSize)), Header(_start, HeaderSize).GetPayloadSize() }; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    void SetPayload(std::span<const PayloadType> payload)
    {
        Header(_start, _size).SetPayloadSize(payload.size());
        if (!payload.empty()) {
            std::memcpy(std::next(_start, HeaderSize), payload.data(), payload.size());
        }
    }

    void SetPayloadSize(std::size_t size)
    {
        Header(_start, _size).SetPayloadSize(size);
    }

private:
    std::byte* _start;
    PayloadSizeType _size;
};

} // namespace FastTransport::Protocol
