#pragma once

#include <mutex>
#include <vector>
#include <list>
#include <memory>

#include "HeaderBuffer.h"
#include "IPacket.h"
#include "LockedList.h"


namespace FastTransport
{
    namespace   Protocol
    {

        class BufferOwner : public IPacket, public FreeableBuffer
        {
        public:
            typedef std::vector<char> ElementType;
            typedef LockedList<ElementType> BufferType;
            typedef std::shared_ptr<BufferOwner> Ptr;
            BufferOwner(const BufferOwner& that) = delete;

            BufferOwner(BufferType& freeBuffers, ElementType&& element) : _freeBuffers(freeBuffers), _element(std::move(element)), _acks(nullptr, 0), _header(nullptr, 0)
            {
                 
            }

            ~BufferOwner()
            {
                std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
                _freeBuffers.push_back(std::move(_element));
            }

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
                auto header = GetHeader();
                if (!header.IsValid())
                    return SelectiveAckBuffer::Acks(nullptr, 0);

                if (header.GetPacketType() != PacketType::SYN_ACK)
                    return SelectiveAckBuffer::Acks(nullptr, 0);
                else
                    return SelectiveAckBuffer::Acks(nullptr, 0);

            }

            virtual HeaderBuffer::Header GetHeader() override
            {
                return HeaderBuffer::Header(_element.data(), _element.size());
            }

            virtual HeaderBuffer::SynAckHeader GetSynAckHeader() override
            {
                return HeaderBuffer::SynAckHeader(_element.data(), _element.size());
            }

            virtual PayloadBuffer::Payload GetPayload() override
            {

                if (_element.size() < HeaderBuffer::SynAckHeader::Size)
                {
                    static PayloadBuffer::Payload empty(nullptr, 0);
                    return empty;
                }

                return PayloadBuffer::Payload(_element.data(), HeaderBuffer::SynAckHeader::Size);
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
        private:
            BufferType& _freeBuffers;
            ElementType _element;

            SelectiveAckBuffer::Acks _acks;
            HeaderBuffer::Header _header;
            ConnectionAddr _addr;
        };


    }
}
