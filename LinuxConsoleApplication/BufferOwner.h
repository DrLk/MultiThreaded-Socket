#pragma once

#include <mutex>
#include <vector>
#include <list>
#include <memory>

#include "HeaderBuffer.h"
#include "IPacket.h"


namespace FastTransport
{
    namespace   Protocol
    {
        template<class Element>
        class LockBuffer : public std::list<Element>
        {
        public:
            std::mutex _mutex;
        };


        class BufferOwner : IPacket
        {
        public:
            typedef std::vector<char> ElementType;
            typedef LockBuffer<ElementType> BufferType;
            BufferOwner(BufferType& freeBuffers) : _freeBuffers(freeBuffers)
            {
                throw std::runtime_error("Not implemented");
            }

            BufferOwner(BufferType& freeBuffers, ElementType&& element) : _freeBuffers(freeBuffers), _element(std::move(element))
            {
            }

            ~BufferOwner()
            {
                std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
                _freeBuffers.push_back(std::move(_element));
            }

            /*void GeneratePacket()
            {
                //HeaderBuffer header(buffer->)
            }*/

            SelectiveAckBuffer GetAcksBuffer() override
            {
                throw std::runtime_error("Not implemented");
            }

            HeaderBuffer GetHeaderBuffer() override
            {
                throw std::runtime_error("Not implemented");
            }

            AddrBuffer GetAddrBuffer() override
            {
                throw std::runtime_error("Not implemented");
            }

            size_t GetBufferSize() const
            {
                return _element.size();
            }

            SelectiveAckBuffer::Acks GetAcks() override
            {
                throw std::runtime_error("Not implemented");
            }

            HeaderBuffer::Header GetHeader() override
            {
                throw std::runtime_error("Not implemented");
            }

            ConnectionAddr GetAddr() override
            {
                throw std::runtime_error("Not implemented");
            }

        private:
            BufferType& _freeBuffers;
            ElementType _element;
        };


    }
}
