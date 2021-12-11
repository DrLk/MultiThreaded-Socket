#include "IInFilghtQueue.h"

using namespace std::chrono_literals;


namespace FastTransport
{
    namespace Protocol
    {
        std::list<std::unique_ptr<IPacket>> IInflightQueue::AddQueue(std::list<OutgoingPacket>&& packets)
        {
            std::list<std::unique_ptr<IPacket>> freePackets;
            std::unordered_set<SeqNumberType> receivedAcks;
            std::unordered_map<SeqNumberType, OutgoingPacket> queue;

            {
                std::lock_guard lock(_receivedAcksMutex);
                receivedAcks.swap(_receivedAcks);
            }

            {
                std::lock_guard lock(_queueMutex);
                queue.swap(_queue);
            }

            for (auto& packet : packets)
            {
                if (packet._needAck)
                {
                    SeqNumberType packetNumber = packet._packet->GetHeader().GetSeqNumber();
                    const auto& ack = receivedAcks.find(packetNumber);
                    if (ack == receivedAcks.end())
                        _queue[packetNumber] = std::move(packet);
                    else
                    {
                        receivedAcks.erase(ack);
                        freePackets.push_back(std::move(packet._packet));
                    }
                }
                else
                    freePackets.push_back(std::move(packet._packet));
            }

            {
                std::lock_guard lock(_receivedAcksMutex);
                _receivedAcks.merge(std::move(receivedAcks));
            }

            {
                std::lock_guard lock(_queueMutex);
                _queue.merge(std::move(queue));
            }

            return freePackets;
        }

        std::list<std::unique_ptr<IPacket>> IInflightQueue::ProcessAcks(const SelectiveAckBuffer::Acks& acks)
        {
            std::list<std::unique_ptr<IPacket>> freePackets;
            std::unordered_set<SeqNumberType> receivedAcks;
            std::unordered_map<SeqNumberType, OutgoingPacket> queue;

            {
                std::lock_guard lock(_queueMutex);
                queue.swap(_queue);
            }

            if (!acks.IsValid())
                throw std::runtime_error("Not Implemented");

            for (SeqNumberType number : acks.GetAcks())
            {
                const auto& it = queue.find(number);
                if (it != queue.end())
                {
                    freePackets.push_back(std::move(it->second._packet));
                    queue.erase(it);
                }
                else
                {
                    receivedAcks.insert(number);
                }
            }

            {
                std::lock_guard lock(_receivedAcksMutex);
                _receivedAcks.merge(std::move(receivedAcks));
            }

            {
                std::lock_guard lock(_queueMutex);
                _queue.merge(std::move(queue));
            }

            return freePackets;
        }

        std::list<OutgoingPacket> IInflightQueue::CheckTimeouts()
        {
            std::list<OutgoingPacket> needToSend;
            std::unordered_map<SeqNumberType, OutgoingPacket> queue;

            {
                std::lock_guard lock(_queueMutex);
                queue.swap(_queue);
            }

            clock::time_point now = clock::now();
            for (auto it = queue.begin(); it != queue.end();)
            {
                OutgoingPacket&& packet = std::move(it->second);
                if ((now - packet._sendTime) > clock::duration(1000ms))
                {
                    needToSend.push_back(std::move(packet));

                    it = queue.erase(it);
                }
                else
                    it++;
            }

            {
                std::lock_guard lock(_queueMutex);
                _queue.merge(std::move(queue));
            }

            return needToSend;
        }
    }
}