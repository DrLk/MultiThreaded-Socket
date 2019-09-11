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

            BufferOwner(BufferType& freeBuffers, ElementType&& element) : _freeBuffers(freeBuffers), _element(std::move(element)), _acks(nullptr, 0), ElementSize(1500)
            {
                 
            }

            ~BufferOwner()
            {
                _element.resize(ElementSize);
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
        private:
            BufferType& _freeBuffers;
            ElementType _element;

            SelectiveAckBuffer::Acks _acks;
            HeaderBuffer::Header _header;
            ConnectionAddr _addr;
            int ElementSize; //TODO: reset size before insert in freeBuffers
        };


    }
}
