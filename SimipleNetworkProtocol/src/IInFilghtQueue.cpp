#include "IInFilghtQueue.h"

namespace FastTransport
{
    namespace Protocol
    {
        void IInflightQueue::AddQueue(std::unique_ptr<IPacket>&& packet)
        {

        }

        void IInflightQueue::AddQueue(std::list<std::unique_ptr<IPacket>>&& packets)
        {

        }


        std::list<std::unique_ptr<IPacket>> IInflightQueue::ProcessAcks(const SelectiveAckBuffer::Acks& acks)
        {
            if (!acks.IsValid())
                throw std::runtime_error("Not Implemented");

            for (SeqNumberType number : acks.GetAcks())
            {
                _queue.erase(number);
            }

            throw std::runtime_error("Not Implemented");
        }
    }
}