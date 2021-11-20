#pragma once
#include "HeaderBuffer.h"
#include "FreeableBuffer.h"
#include "ConnectionAddr.h"

namespace FastTransport
{
    namespace Protocol
    {
        class IPacket
        {

        public:
            virtual SelectiveAckBuffer GetAcksBuffer()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual HeaderBuffer GetHeaderBuffer()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual HeaderBuffer::SynAckHeader GetSynAckHeader()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual AddrBuffer GetAddrBuffer()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual SelectiveAckBuffer::Acks GetAcks()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual HeaderBuffer::Header GetHeader()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual ConnectionAddr GetAddr()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual PayloadBuffer::Payload GetPayload()
            {
                throw std::runtime_error("Not implemented");
            }

            virtual void Copy(const IPacket& packet) = 0;
        };

        class TestPacket : public IPacket, public FreeableBuffer
        {
        private:
            SelectiveAckBuffer::Acks _acks;
            HeaderBuffer::Header _header;
            ConnectionAddr _addr;

        public:
            virtual SelectiveAckBuffer GetAcksBuffer() override
            {
                return SelectiveAckBuffer(_acks, this->shared_from_this());
            }

            virtual HeaderBuffer GetHeaderBuffer() override
            {
                return HeaderBuffer(_header, this->shared_from_this());
            }

            virtual AddrBuffer GetAddrBuffer() override
            {
                return AddrBuffer(_addr, this->shared_from_this());
            }

            virtual SelectiveAckBuffer::Acks GetAcks() override
            {
                return _acks;
            }

            virtual HeaderBuffer::Header GetHeader() override
            {
                return _header;
            }

            virtual ConnectionAddr GetAddr() override
            {
                return _addr;
            }

            void SetAcks(const SelectiveAckBuffer::Acks& acks)
            {
                _acks = acks;
            }

            void SetHeader(const HeaderBuffer::Header& header)
            {
                _header = header;
            }

            void SetAddr(const ConnectionAddr& addr)
            {
                _addr = addr;
            }
        };
    }
}
