#pragma once

#include <memory>
#include <vector>
#include <list>
#include <span>

#include "ConnectionAddr.h"

namespace FastTransport
{
    namespace Protocol
    {
        typedef unsigned short ConnectionID;
        typedef unsigned int SeqNumberType;
        typedef int MagicNumber;

        enum class PacketType : short
        {
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
        class Header : protected std::span<char>
        {
        public:
            Header(char* start, size_t size) : std::span<char>(start, size)
            {
            }

            static const int Size = sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType) + sizeof(SeqNumberType);

            bool IsValid() const
            {
                if (size() < Size)
                    return false;

                return *reinterpret_cast<const MagicNumber*>(data()) == Magic_Number;
            }
            PacketType GetPacketType() const
            {
                return *reinterpret_cast<const PacketType*>(data() + sizeof(MagicNumber));
            }
            ConnectionID GetSrcConnectionID() const
            {
                return *reinterpret_cast<const ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType));
            }
            ConnectionID GetDstConnectionID() const
            {
                return *reinterpret_cast<const ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID));
            }
            SeqNumberType GetSeqNumber() const
            {
                return *reinterpret_cast<const SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID));
            }
            SeqNumberType GetAckNumber() const
            {
                return *reinterpret_cast<const SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType));
            }
            void SetMagic()
            {
                *reinterpret_cast<MagicNumber*>(const_cast<char*>(data())) = Magic_Number;
            }
            void SetPacketType(PacketType type)
            {
                *reinterpret_cast<PacketType*>(const_cast<char*>(data() + sizeof(MagicNumber))) = type;
            }
            void SetSrcConnectionID(ConnectionID id)
            {
                *reinterpret_cast<ConnectionID*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType))) = id;
            }

            void SetDstConnectionID(ConnectionID id)
            {
                *reinterpret_cast<ConnectionID*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType) +sizeof(ConnectionID))) = id;
            }

            void SetSeqNumber(SeqNumberType seq)
            {
                *reinterpret_cast<SeqNumberType*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID))) = seq;
            }

            void SetAckNumber(SeqNumberType ack)
            {
                *reinterpret_cast<SeqNumberType*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(ConnectionID) + sizeof(SeqNumberType))) = ack;
            }
        };

        class SelectiveAckBuffer
        {
        public:
            class Acks
            {
            public:
                static const SeqNumberType MaxAcks = 300;

                Acks(char* start, size_t size) : _start(0), _size(0)
                {
                    size_t ackPacketStart = Header::Size + sizeof(MaxAcks);
                    if (size >= ackPacketStart)
                        _size = *reinterpret_cast<SeqNumberType*>(start + Header::Size);
                    if ((unsigned long long)size >= (Header::Size + sizeof(MaxAcks) + _size *sizeof(SeqNumberType)))
                        _start = reinterpret_cast<SeqNumberType*>(start + Header::Size);
                    else
                        _size = 0;
                }

                std::basic_string_view<SeqNumberType> GetAcks() const
                {
                    return std::basic_string_view(_start + 1, _size);
                }

                void SetAcks(const std::list<SeqNumberType>& numbers)
                {
                    //TODO: check size
                    _size = 0;
                    *_start = (SeqNumberType)numbers.size();
                    for (auto number : numbers)
                    {
                        _size++;
                        *(_start + _size) = number;
                    }
                }

                bool IsValid() const
                {
                    if (_size > 0)
                        return true;

                    return false;
                }
            private:
                SeqNumberType* _start;
                int _size;
            };

            SelectiveAckBuffer(const Acks& acks) : _acks(acks)
            {
            }

            Acks _acks;
        };

        class PayloadBuffer
        {
        public:
            typedef char PayloadType;
            class Payload : public std::basic_string_view<PayloadType>
            {
            public:
                static const SeqNumberType MaxPayload = 1300;
                Payload(PayloadType* start, size_t size) : _start(0), _size(0)
                {
                    size_t ackPacketStart = Header::Size + sizeof(MaxPayload);
                    if (size >= ackPacketStart)
                        _size = *reinterpret_cast<PayloadType*>(start + Header::Size);
                    if ((unsigned long long)size >= (Header::Size + sizeof(MaxPayload) + _size))
                        _start = reinterpret_cast<PayloadType*>(start + Header::Size);
                    else
                        _size = 0;
                }

                std::basic_string_view<PayloadType> GetPayload() const
                {
                    return std::basic_string_view<PayloadType>(_start, _size);
                }

                void SetPayload(const std::vector<PayloadType>& payload)
                {
                    //TODO: check size
                    _size = (int)payload.size();
                    std::memcpy(_start + sizeof(_size), payload.data(), payload.size());
                }

            private:
                PayloadType* _start;
                unsigned int _size;
            };

            PayloadBuffer(PayloadType* start, int count) : _payload(start, count)
            {
            }

            Payload _payload;
        };

        class AddrBuffer
        {
        public:
            AddrBuffer(const ConnectionAddr& addr) : _dstAddr(addr)
            {
            }

            const ConnectionAddr& GetDstAddr() const
            {
                return _dstAddr;
            }

            ConnectionAddr _dstAddr;
        };

    }
}
