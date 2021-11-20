#include "IInFilghtQueue.h"

namespace FastTransport
{
    namespace Protocol
    {
        void IInflightQueue::AddQueue(std::unique_ptr<OutgoingPacket>&& packet)
        {

        }

        void IInflightQueue::AddQueue(std::list<OutgoingPacket>&& packets)
        {

        }

        std::list<std::unique_ptr<OutgoingPacket>> IInflightQueue::ProcessAcks(const SelectiveAckBuffer::Acks& acks)
        {
            if (!acks.IsValid())
                throw std::runtime_error("Not Implemented");

            for (SeqNumberType number : acks.GetAcks())
            {
                _queue.erase(number);
            }

            throw std::runtime_error("Not Implemented");
        }

        std::list<OutgoingPacket> IInflightQueue::CheckTimeouts()
        {
            std::list<OutgoingPacket> needToSend;
            TimePoint now = Time::now();
            for (auto& pair : _queue)
            {
                OutgoingPacket&& packet = std::move(pair.second);
                if (std::chrono::duration_cast<ms>(now - packet._sendTime) > ms(10))
                {
                    needToSend.push_back(std::move(packet));
                }
            }

            return needToSend;
        }
    }
}