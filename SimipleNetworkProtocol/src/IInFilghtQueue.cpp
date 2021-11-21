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
            for (auto& packet : packets)
            {
                _queue[packet._packet->GetHeader().GetSeqNumber()] = std::move(packet);
            }
        }

        void IInflightQueue::ProcessAcks(const SelectiveAckBuffer::Acks& acks)
        {
            if (!acks.IsValid())
                throw std::runtime_error("Not Implemented");

            for (SeqNumberType number : acks.GetAcks())
            {
                _queue.erase(number);
            }
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