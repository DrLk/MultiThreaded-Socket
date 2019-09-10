#pragma once

#include <memory>



namespace FastTransport
{
    namespace Protocol
    {
        class BufferOwner;

        class FreeableBuffer
        {
        public:
            FreeableBuffer(std::shared_ptr<BufferOwner>& buffer) : _buffer(buffer)
            {
            }
            FreeableBuffer(std::shared_ptr<BufferOwner>&& buffer) noexcept : _buffer(std::move(buffer))
            {
            }
            virtual ~FreeableBuffer()
            {
            }
        protected:
            std::shared_ptr<BufferOwner> _buffer;
        };


        typedef unsigned short ConnectionIDType;
        typedef unsigned int SeqNumberType;
        typedef int MagicNumber;

        enum PacketType : short
        {
            NONE = 0,
            SYN = 1,
            ACK = 2,
            SYN_ACK = 3,

        };



        class HeaderBuffer : public FreeableBuffer
        {
        public:
            class Header
            {
            public:
                Header()
                {
                }


                MagicNumber _magic;
                PacketType _packetType;
                ConnectionIDType _connectionID;
                SeqNumberType _seqNumber;
            };

            HeaderBuffer(Header* start, int size, std::shared_ptr<BufferOwner>& buffer) : FreeableBuffer(buffer), _header(start)
            {
            }
            HeaderBuffer(Header* start, int size, std::shared_ptr<BufferOwner>&& buffer) : FreeableBuffer(std::move(buffer)), _header(start)
            {
            }

            void SetHeader(PacketType packetType, ConnectionIDType connectionID, SeqNumberType seqNumber);
            bool IsValid() const;
            ConnectionIDType GetConnectionID() const;
        private:
            Header* _header;
        };

        class SelectiveAckBuffer : public FreeableBuffer
        {
        public:
            class Acks : private std::basic_string_view<SeqNumberType>
            {
            public:
                Acks(SeqNumberType* start, int count) : std::basic_string_view<SeqNumberType>(start, count)
                {
                }
            };

            SelectiveAckBuffer(SeqNumberType* start, int count, std::shared_ptr<BufferOwner>& buffer) : FreeableBuffer(buffer), _acks(start, count)
            {
            }
            SelectiveAckBuffer(SeqNumberType* start, int count, std::shared_ptr<BufferOwner>&& buffer) : FreeableBuffer(std::move(buffer)), _acks(start, count)
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
            PayloadBuffer(PayloadType* start, int count, std::shared_ptr<BufferOwner>& buffer) : FreeableBuffer(buffer), _payload(start, count)
            {
            }
            PayloadBuffer(PayloadType* start, int count, std::shared_ptr<BufferOwner>&& buffer) : FreeableBuffer(std::move(buffer)), _payload(start, count)
            {
            }

            Payload _payload;
        };

        class FastProtocolPacket : public FreeableBuffer
        {
        public:
            FastProtocolPacket(std::shared_ptr<BufferOwner>& buffer) : FreeableBuffer(buffer)
            {

            }

            void GenerateSendPacket()
            {

            }

        };
    }
}
