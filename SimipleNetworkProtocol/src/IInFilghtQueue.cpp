#include "IInFilghtQueue.h"

using namespace std::chrono_literals;


namespace FastTransport
{
    namespace Protocol
    {
        std::list<std::unique_ptr<IPacket>> IInflightQueue::AddQueue(std::list<OutgoingPacket>&& packets)
        {
            std::list<std::unique_ptr<IPacket>> freePackets;

            for (auto& packet : packets)
            {
                if (packet._needAck)
                    _queue[packet._packet->GetHeader().GetSeqNumber()] = std::move(packet);
                else
                    freePackets.push_back(std::move(packet._packet));
            }

            return freePackets;
        }

        std::list<std::unique_ptr<IPacket>> IInflightQueue::ProcessAcks(const SelectiveAckBuffer::Acks& acks)
        {
            std::list<std::unique_ptr<IPacket>> freePackets;

            if (!acks.IsValid())
                throw std::runtime_error("Not Implemented");

            for (SeqNumberType number : acks.GetAcks())
            {
                const auto& it = _queue.find(number);
                if (it != _queue.end())
                {
                    freePackets.push_back(std::move(it->second._packet));
                    _queue.erase(it);
                }
            }

            return freePackets;
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