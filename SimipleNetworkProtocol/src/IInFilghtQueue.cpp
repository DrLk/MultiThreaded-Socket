#include "IInFilghtQueue.h"

namespace FastTransport
{
    namespace Protocol
    {
        using namespace std::chrono_literals;

        void IInflightQueue::AddQueue(OutgoingPacket&& packet)
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
            clock::time_point now = clock::now();
            for (auto it = _queue.begin(); it != _queue.end();)
            {
                OutgoingPacket&& packet = std::move(it->second);
                if ((now - packet._sendTime) > clock::duration(10ms))
                {
                    needToSend.push_back(std::move(packet));

                    it = _queue.erase(it);
                }
                else
                    it++;
            }

            return needToSend;
        }
    }
}