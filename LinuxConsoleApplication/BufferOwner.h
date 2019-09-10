#pragma once

#include <mutex>
#include <vector>
#include <list>
#include <memory>

#include "HeaderBuffer.h"


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
        class BufferOwner
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

            SelectiveAckBuffer GetAcks() const
            {
                throw std::runtime_error("Not implemented");
            }

            HeaderBuffer GetHeader() const
            {
                throw std::runtime_error("Not implemented");
            }

            size_t GetBufferSize() const
            {
                return _element.size();
            }

        private:
            BufferType& _freeBuffers;
            ElementType _element;
        };


    }
}
