#pragma once

#include <memory>
#include <vector>
#include <list>

#include "ConnectionAddr.h"
#include "FreeableBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {
        typedef unsigned short ConnectionID;
        typedef unsigned int SeqNumberType;
        typedef int MagicNumber;

        enum PacketType : short
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
        class HeaderBuffer : public FreeableBuffer
        {
        public:

            class Header : protected std::basic_string_view<char>
            {
            public:
                Header(char* start, size_t size) : std::basic_string_view<char>(start, size)
                {
                }

                static const int Size = sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID) + sizeof(SeqNumberType);

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
                ConnectionID GetConnectionID() const
                {
                    return *reinterpret_cast<const ConnectionID*>(data() + sizeof(MagicNumber) + sizeof(PacketType));
                }
                SeqNumberType GetSeqNumber() const
                {
                    return *reinterpret_cast<const SeqNumberType*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID));
                }
                void SetMagic()
                {
                    *reinterpret_cast<MagicNumber*>(const_cast<char*>(data())) = Magic_Number;
                }
                void SetPacketType(PacketType type)
                {
                    *reinterpret_cast<PacketType*>(const_cast<char*>(data() + sizeof(MagicNumber))) = type;
                }
                void SetConnectionID(ConnectionID id)
                {
                    *reinterpret_cast<ConnectionID*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType))) = id;
                }
                void SetSeqNumber(SeqNumberType seq)
                {
                    *reinterpret_cast<SeqNumberType*>(const_cast<char*>(data() + sizeof(MagicNumber) + sizeof(PacketType) + sizeof(ConnectionID))) = seq;
                }
            };

            class SynAckHeader : public Header
            {
            public:
                static const int Size = Header::Size + sizeof(ConnectionID);
                SynAckHeader(char* start, size_t size) : Header(start, size)
                {
                }

                ConnectionID GetRemoteConnectionID() const
                {
                    return *reinterpret_cast<const ConnectionID*>(data() + Header::Size);
                }

                void SetRemoteConnectionID(ConnectionID seq)
                {
                    *reinterpret_cast<ConnectionID*>(const_cast<char*>(data() + Header::Size)) = seq;
                }

            };

            HeaderBuffer(const Header& header, std::shared_ptr<FreeableBuffer>& buffer) : FreeableBuffer(buffer), _header(header)
            {
            }
            HeaderBuffer(const Header& header, std::shared_ptr<FreeableBuffer>&& buffer) : FreeableBuffer(std::move(buffer)), _header(header)
            {
            }

            void SetHeader(PacketType packetType, ConnectionID connectionID, SeqNumberType seqNumber);
            bool IsValid() const;
            ConnectionID GetConnectionID() const;
            PacketType GetType() const;
            SeqNumberType GetSeqNumber() const;

        private:
            Header _header;
        };

        class SelectiveAckBuffer : public FreeableBuffer
        {
        public:
            class Acks
            {
            public:
                static const SeqNumberType MaxAcks = 300;

                Acks(char* start, size_t size) : _start(0), _size(0)
                {
                    size_t ackPacketStart = HeaderBuffer::Header::Size + sizeof(MaxAcks);
                    if (size >= ackPacketStart)
                        _size = *reinterpret_cast<SeqNumberType*>(start + HeaderBuffer::Header::Size);
                    if ((unsigned long long)size >= (HeaderBuffer::Header::Size + sizeof(MaxAcks) + _size *sizeof(SeqNumberType)))
                        _start = reinterpret_cast<SeqNumberType*>(start + HeaderBuffer::Header::Size);
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

            SelectiveAckBuffer(const Acks& acks, std::shared_ptr<FreeableBuffer>& buffer) : FreeableBuffer(buffer), _acks(acks)
            {
            }
            SelectiveAckBuffer(const Acks& acks, std::shared_ptr<FreeableBuffer>&& buffer) : FreeableBuffer(std::move(buffer)), _acks(acks)
            {
            }

            Acks _acks;
        };

        class PayloadBuffer : public FreeableBuffer
        {
        public:
            typedef char PayloadType;
            class Payload : public std::basic_string_view<PayloadType>
            {
            public:
                static const SeqNumberType MaxPayload = 1300;
                Payload(PayloadType* start, size_t size) : _start(0), _size(0)
                {
                    size_t ackPacketStart = HeaderBuffer::Header::Size + sizeof(MaxPayload);
                    if (size >= ackPacketStart)
                        _size = *reinterpret_cast<PayloadType*>(start + HeaderBuffer::Header::Size);
                    if ((unsigned long long)size >= (HeaderBuffer::Header::Size + sizeof(MaxPayload) + _size))
                        _start = reinterpret_cast<PayloadType*>(start + HeaderBuffer::Header::Size);
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

            PayloadBuffer(PayloadType* start, int count, std::shared_ptr<FreeableBuffer>& buffer) : FreeableBuffer(buffer), _payload(start, count)
            {
            }
            PayloadBuffer(PayloadType* start, int count, std::shared_ptr<FreeableBuffer>&& buffer) : FreeableBuffer(std::move(buffer)), _payload(start, count)
            {
            }

            Payload _payload;
        };

        class AddrBuffer : public FreeableBuffer
        {
        public:
            AddrBuffer(const ConnectionAddr& addr, std::shared_ptr<FreeableBuffer>& buffer) : FreeableBuffer(buffer), _dstAddr(addr)
            {
            }
            AddrBuffer(const ConnectionAddr& addr, std::shared_ptr<FreeableBuffer>&& buffer) : FreeableBuffer(std::move(buffer)), _dstAddr(addr)
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
