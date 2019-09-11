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
            FIN = 4,
            CLOSE = 8,

        };

        class HeaderBuffer : public FreeableBuffer
        {
        public:
            class Header : public std::basic_string_view<char>
            {
            public:
                Header()
                {
                }


                const MagicNumber& GetMagic() const
                {
                    return *reinterpret_cast<const MagicNumber*>(data());
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
                void SetMagic(MagicNumber magic)
                {
                    *reinterpret_cast<MagicNumber*>(const_cast<char*>(data())) = magic;
                }
                void SetPacketType(PacketType type)
                {
                    *reinterpret_cast<PacketType*>(const_cast<char*>(data())) = type;
                }
                void SetConnectionID(ConnectionID id)
                {
                    *reinterpret_cast<ConnectionID*>(const_cast<char*>(data())) = id;
                }
                void SetSeqNumber(SeqNumberType seq)
                {
                    *reinterpret_cast<SeqNumberType*>(const_cast<char*>(data())) = seq;
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
            class Acks : public std::basic_string_view<SeqNumberType>
            {
            public:
                Acks(SeqNumberType* start, int count) : std::basic_string_view<SeqNumberType>(start, count)
                {
                }
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
            typedef char PayloadType;
            class Payload : private std::basic_string_view<PayloadType>
            {
            public:
                Payload(PayloadType* start, int count) : std::basic_string_view<PayloadType>(start, count)
                {
                }
            };
        public:
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
