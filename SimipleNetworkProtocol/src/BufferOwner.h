#pragma once

#include <mutex>
#include <vector>
#include <list>
#include <memory>
#include <chrono>

#include "HeaderBuffer.h"
#include "IPacket.h"
#include "LockedList.h"


namespace FastTransport
{
    namespace   Protocol
    {

        class BufferOwner : public IPacket
        {
        public:
            typedef std::vector<char> ElementType;
            typedef LockedList<ElementType> BufferType;
            typedef std::shared_ptr<BufferOwner> Ptr;
            BufferOwner(const BufferOwner& that) = delete;

            BufferOwner(BufferType& freeBuffers, ElementType&& element) : /*_freeBuffers(freeBuffers),*/ _element(std::move(element))
            {
                 
            }

            BufferOwner(int size) : _element(size)
            {

            }

            ~BufferOwner()
            {
                //std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
                //_freeBuffers.push_back(std::move(_element));
            }

            virtual SelectiveAckBuffer GetAcksBuffer() override
            {
                throw std::runtime_error("Not Implemented");
            }

            virtual HeaderBuffer GetHeaderBuffer() override
            {
                throw std::runtime_error("Not Implemented");
            }

            virtual SelectiveAckBuffer::Acks GetAcks() override
            {
                return SelectiveAckBuffer::Acks(_element.data(), _element.size());
            }

            virtual HeaderBuffer::Header GetHeader() override
            {
                return HeaderBuffer::Header(_element.data(), _element.size());
            }

            virtual PayloadBuffer::Payload GetPayload() override
            {
                return PayloadBuffer::Payload(_element.data(), _element.size());
            }

            virtual ConnectionAddr GetDstAddr() override
            {
                return _dstAddr;
            }

            void SetAcks(const SelectiveAckBuffer::Acks& acks)
            {
                throw std::runtime_error("Not Implemented");
            }

            void SetHeader(const HeaderBuffer::Header& header)
            {
                throw std::runtime_error("Not Implemented");
            }

            void SetAddr(const ConnectionAddr& addr)
            {
                _dstAddr = addr;
            }

            std::chrono::nanoseconds GetTime() const
            {
                return _time;
            }

            void SetTime(const std::chrono::nanoseconds& time)
            {
                _time = time;
            }

            virtual void Copy(const IPacket& packet) override
            {
                const BufferOwner& that = dynamic_cast<const BufferOwner&>(packet);

                _dstAddr = that._dstAddr;
                _time = that._time;
                _element = that._element;
            }

        private:
            ElementType _element;

            ConnectionAddr _srcAddr;
            ConnectionAddr _dstAddr;
            std::chrono::nanoseconds _time;
        };


    }
}
