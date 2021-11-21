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

        class Packet : public IPacket
        {
        public:
            typedef std::vector<char> ElementType;
            typedef LockedList<ElementType> BufferType;
            typedef std::shared_ptr<Packet> Ptr;
            Packet(const Packet& that) = delete;

            Packet(BufferType& freeBuffers, ElementType&& element) : _element(std::move(element))
            {
                 
            }

            Packet(int size) : _element(size)
            {

            }

            ~Packet()
            {
            }

            virtual SelectiveAckBuffer GetAcksBuffer() override
            {
                throw std::runtime_error("Not Implemented");
            }

            virtual SelectiveAckBuffer::Acks GetAcks() override
            {
                return SelectiveAckBuffer::Acks(_element.data(), _element.size());
            }

            virtual Header GetHeader() override
            {
                return Header(_element.data(), _element.size());
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

            void SetHeader(const Header& header)
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
                const Packet& that = dynamic_cast<const Packet&>(packet);

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
