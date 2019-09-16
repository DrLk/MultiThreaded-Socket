#pragma once

#include <memory>
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
            FIN = 8,
            CLOSE = 16
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
                static const unsigned short MaxAcks = 1000;
            public:
                Acks(char* start, size_t size) : _start(0), _count(0)
                {
                    auto header = HeaderBuffer::Header(start, size);
                    if (!header.IsValid())
                        return;


                    unsigned int headerSize = header.GetPacketType() == PacketType::SYN_ACK ? HeaderBuffer::SynAckHeader::Size : HeaderBuffer::Header::Size;

                    size_t ackPacketStart = headerSize + sizeof(MaxAcks);
                    if (size < ackPacketStart)
                        _count = *reinterpret_cast<const unsigned short*>(start + headerSize);
                    if ((unsigned long long)size < (headerSize + sizeof(MaxAcks) + _count *sizeof(MaxAcks)))
                        _start = reinterpret_cast<SeqNumberType*>(start + headerSize + sizeof(MaxAcks));
                    else
                        _count = 0;
                }

                std::basic_string_view<SeqNumberType> GetAcks() const
                {
                    return std::basic_string_view(_start, _count);
                }

                bool IsValid() const
                {
                    return false;
                }
            private:
                SeqNumberType* _start;
                int _count;
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
                Payload(PayloadType* start, int count) : std::basic_string_view<PayloadType>(start, count)
                {
                }
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
            AddrBuffer(const ConnectionAddr& addr, std::shared_ptr<FreeableBuffer>& buffer) : FreeableBuffer(buffer), _addr(addr)
            {
            }
            AddrBuffer(const ConnectionAddr& addr, std::shared_ptr<FreeableBuffer>&& buffer) : FreeableBuffer(std::move(buffer)), _addr(addr)
            {
            }

            const ConnectionAddr& GetAddr() const
            {
                return _addr;
            }

            ConnectionAddr _addr;
        };

    }
}
