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
            typedef LockedList<ElementType> BufferType;
            typedef std::shared_ptr<Packet> Ptr;
            Packet(const Packet& that) = delete;

            Packet(int size) : _element(size), _time(0)
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

            //TODO: remove and use GetDstAddr for assign
            virtual void SetAddr(const ConnectionAddr& addr) override
            {
                _dstAddr = addr;
            }

            void SetAcks(const SelectiveAckBuffer::Acks& acks)
            {
                throw std::runtime_error("Not Implemented");
            }

            void SetHeader(const Header& header)
            {
                throw std::runtime_error("Not Implemented");
            }

            std::chrono::nanoseconds GetTime() const
            {
                return _time;
            }

            void SetTime(const std::chrono::nanoseconds& time)
            {
                _time = time;
            }

            ElementType& GetElement() override
            {
                return _element;
            }

            virtual void Copy(const IPacket& packet) override
            {
                const Packet& that = dynamic_cast<const Packet&>(packet);

                _srcAddr = that._srcAddr;
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
