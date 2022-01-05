#pragma once

#include <list>
#include <memory>
#include <span>
#include <vector>

#include "ConnectionAddr.hpp"

namespace FastTransport::Protocol {
using ConnectionID = unsigned short;
using SeqNumberType = unsigned int;
using MagicNumber = int;

enum class PacketType : short {
    NONE = 0,
    SYN = 1,
    ACK = 2,
    SYN_ACK = 3,
    DATA = 4,
    SACK = 8,
    FIN = 16,
    CLOSE = 32
};

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
    void SetSrcConnectionID(ConnectionID id)
    {
        *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType)) = id; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    void SetDstConnectionID(ConnectionID id)
    {
        *reinterpret_cast<ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID)) = id; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
        static constexpr SeqNumberType MaxAcks = 300;

        Acks(unsigned char* start, size_t size)
            : _start(nullptr)
            , _size(0)
        {
            size_t ackPacketStart = Header::Size + sizeof(MaxAcks);
            if (size >= ackPacketStart) {
                _size = *reinterpret_cast<SeqNumberType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (static_cast<unsigned long long>(size) >= (Header::Size + sizeof(MaxAcks) + _size * sizeof(SeqNumberType))) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                _start = reinterpret_cast<SeqNumberType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            } else {
                _size = 0;
            }
        }

        [[nodiscard]] std::span<SeqNumberType> GetAcks() const
        {
            return { _start + 1, _size }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        void SetAcks(const std::list<SeqNumberType>& numbers)
        {
            // TODO: check size
            _size = 0;
            std::span<SeqNumberType> acks(_start, MaxAcks);
            acks[0] = static_cast<SeqNumberType>(numbers.size());
            for (auto number : numbers) {
                _size++;
                acks[_size] = number;
            }
        }

        [[nodiscard]] bool IsValid() const
        {
            return _size > 0;
        }

    private:
        SeqNumberType* _start;
        size_t _size;
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
    using PayloadType = unsigned char;
    class Payload : public std::basic_string_view<PayloadType> {
    public:
        static const SeqNumberType MaxPayload = 1300;
        Payload(PayloadType* start, size_t size)
            : _start(nullptr)
            , _size(0)
        {
            size_t ackPacketStart = Header::Size + sizeof(MaxPayload);
            if (size >= ackPacketStart) {
                _size = *reinterpret_cast<PayloadType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (static_cast<unsigned long long>(size) >= (Header::Size + sizeof(MaxPayload) + _size)) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
                _start = reinterpret_cast<PayloadType*>(start + Header::Size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            } else {
                _size = 0;
            }
        }

        [[nodiscard]] std::basic_string_view<PayloadType> GetPayload() const
        {
            return { _start, _size };
        }

        void SetPayload(const std::vector<PayloadType>& payload)
        {
            // TODO: check size
            _size = payload.size();
            std::memcpy(_start + sizeof(_size), payload.data(), payload.size()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

    private:
        PayloadType* _start;
        size_t _size;
    };

    PayloadBuffer(PayloadType* start, int count)
        : _payload(start, count)
    {
    }

private:
    Payload _payload;
};

} // namespace FastTransport::Protocol