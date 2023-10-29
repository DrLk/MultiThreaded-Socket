#pragma once

#include <cstdint>
#include <cstring>
#include <list>
#include <span>
#include <string_view>
#include <vector>

#include "HeaderTypes.hpp"

namespace FastTransport::Protocol {
using MagicNumber = int;

const MagicNumber Magic_Number = 0x12345678;
class Header : protected std::span<unsigned char> {
public:
    Header(unsigned char* start, size_t size)
        : std::span<unsigned char>(start, size)
    {
    }

    static const int Size = sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType);

    [[nodiscard]] bool IsValid() const
    {
        if (size() < Size) {
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
};

class SelectiveAckBuffer {
public:
    class Acks {
    public:
        Acks(unsigned char* start, size_t size)
        {
            const size_t ackPacketStart = Header::Size + sizeof(MaxAcksSize);
            if (size >= ackPacketStart) {
                _size = *reinterpret_cast<SeqNumberType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (static_cast<uint64_t>(size) >= (Header::Size + sizeof(MaxAcksSize) + _size * sizeof(SeqNumberType))) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                _start = reinterpret_cast<SeqNumberType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            } else {
                _size = 0;
            }
        }

        [[nodiscard]] std::span<SeqNumberType> GetAcks() const
        {
            return { _start + sizeof(SeqNumberType), _size }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        void SetAcks(std::span<const SeqNumberType> acks)
        {
            _size = acks.size();
            *reinterpret_cast<SeqNumberType*>(_start) = static_cast<PayloadSizeType>(_size);
            std::copy(acks.begin(), acks.end(), _start + sizeof(PayloadSizeType));
        }

        [[nodiscard]] bool IsValid() const
        {
            return _size > 0;
        }

    private:
        SeqNumberType* _start { nullptr };
        size_t _size { 0 };
    };

    explicit SelectiveAckBuffer(const Acks& acks)
        : _acks(acks)
    {
    }

private:
    Acks _acks;
};

class PayloadBuffer {
public:
    class Payload {
    public:
        Payload(PayloadType* start, size_t size)

        {
            const size_t payloadStart = Header::Size + sizeof(PayloadSizeType);
            if (size >= payloadStart) {
                _size = *reinterpret_cast<PayloadSizeType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (static_cast<uint64_t>(size) >= (Header::Size + sizeof(PayloadSizeType) + _size)) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                _start = reinterpret_cast<PayloadType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            } else {
                _size = 0;
            }
        }

        [[nodiscard]] std::span<PayloadType> GetPayload() const
        {
            return { _start + sizeof(PayloadSizeType), _size }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        void SetPayload(std::span<PayloadType> payload)
        {
            _size = payload.size();
            *reinterpret_cast<PayloadSizeType*>(_start) = static_cast<PayloadSizeType>(_size);
            std::memcpy(_start + sizeof(PayloadSizeType), payload.data(), payload.size()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

    private:
        PayloadType* _start { nullptr };
        size_t _size { 0 };
    };

    PayloadBuffer(PayloadType* start, int count)
        : _payload(start, count)
    {
    }

private:
    Payload _payload;
};

} // namespace FastTransport::Protocol
