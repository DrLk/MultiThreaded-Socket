#pragma once

#include <vector>
#include <memory>

#include "BufferOwner.h"
#include "HeaderBuffer.h"

namespace FastTransport
{
    namespace Protocol
    {

        class IRecvQueue
        {
        public:
            virtual ~IRecvQueue() { }
            virtual void AddPacket(BufferOwner::Ptr& packet) = 0;
            virtual std::vector<char> GetUserData() = 0;
            virtual std::vector<SeqNumberType> GetSelectiveAcks() = 0;

        public:
        };

        class RecvQueue : public IRecvQueue
        {



        virtual void AddPacket(BufferOwner::Ptr& packet) override
        {
            _selectiveAcks.push_back(packet->GetHeader().GetSeqNumber());
        }

        virtual std::vector<char> GetUserData() override
        {
            throw std::runtime_error("Not implemented");
        }

        virtual std::vector<SeqNumberType> GetSelectiveAcks() override
        {
            return _selectiveAcks;
        }

        std::vector<BufferOwner::Ptr> _packets;
        std::vector<SeqNumberType> _selectiveAcks;

        };

    }
}