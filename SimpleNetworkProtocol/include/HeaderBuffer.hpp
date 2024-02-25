#pragma once

#include "cstddef"
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

        return *reinterpret_cast<MagicNumber*>(data()) == Magic_Number; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }

    [[nodiscard]] PacketType GetPacketType() const
    {
        return *reinterpret_cast<PacketType*>(data() + sizeof(MagicNumber)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] ConnectionID GetSrcConnectionID() const
    {
        return *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] ConnectionID GetDstConnectionID() const
    {
        return *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] SeqNumberType GetSeqNumber() const
    {
        return *reinterpret_cast<SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] SeqNumberType GetAckNumber() const
    {
        return *reinterpret_cast<SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] PayloadSizeType GetPayloadSize() const
    {
        return *reinterpret_cast<PayloadSizeType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetMagic()
    {
        *reinterpret_cast<MagicNumber*>(data()) = Magic_Number; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetPacketType(PacketType type)
    {
        *reinterpret_cast<PacketType*>(data() + sizeof(MagicNumber)) = type; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetSrcConnectionID(ConnectionID connectionId)
    {
        *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType)) = connectionId; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetDstConnectionID(ConnectionID connectionId)
    {
        *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID)) = connectionId; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetSeqNumber(SeqNumberType seq)
    {
        *reinterpret_cast<SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID)) = seq; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetAckNumber(SeqNumberType ack)
    {
        *reinterpret_cast<SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType)) = ack; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetPayloadSize(PayloadSizeType payloadSize)
    {
        *reinterpret_cast<PayloadSizeType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType)) = payloadSize; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
        return { reinterpret_cast<SeqNumberType*>(_start + HeaderSize), Header(_start, _size).GetPayloadSize() / sizeof(SeqNumberType) }; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetAcks(std::span<const SeqNumberType> acks)
    {
        Header(_start, _size).SetPayloadSize(acks.size() * sizeof(SeqNumberType));
        std::copy(acks.begin(), acks.end(), reinterpret_cast<SeqNumberType*>(_start + HeaderSize)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
        return { reinterpret_cast<PayloadType*>(_start + HeaderSize), Header(_start, HeaderSize).GetPayloadSize() }; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetPayload(std::span<PayloadType> payload)
    {
        Header(_start, _size).SetPayloadSize(payload.size());
        std::memcpy(_start + HeaderSize, payload.data(), payload.size()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
