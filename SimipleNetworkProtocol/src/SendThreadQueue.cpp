#include "SendThreadQueue.h"

#include <thread>

#include "UDPQueue.h"
#include "IPacket.h"

using namespace std::chrono_literals;


namespace FastTransport::Protocol
{
    SendThreadQueue::SendThreadQueue() : _pauseTime(1)
    {
    }


    void SendThreadQueue::WriteThread(UDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, unsigned short index)
    {
        std::list<OutgoingPacket> sendQueue;

        bool sleep = false;

        while (true)
        {
            if (sleep)
                std::this_thread::sleep_for(1ms);

            if (sendQueue.empty())
            {
                std::lock_guard lock(udpQueue._sendQueue._mutex);
                if (udpQueue._sendQueue.empty())
                {
                    sleep = true;
                    continue;
                }
                else
                {
                    sleep = false;
                }

                if (udpQueue._sendQueueSizePerThread < udpQueue._sendQueue.size())
                {
                    auto end = udpQueue._sendQueue.begin();
                    for (size_t i = 0; i < udpQueue._sendQueueSizePerThread; i++)
                    {
                        end++;
                    }
                    sendQueue.splice(sendQueue.begin(), udpQueue._sendQueue, udpQueue._sendQueue.begin(), end);
                }
                else
                {
                    sendQueue.swap(udpQueue._sendQueue);
                }
            }

            for (auto& packet : sendQueue)
            {
                unsigned short currentPort = (unsigned short)(ntohs(packet._packet->GetDstAddr().GetPort()) + (unsigned short)index);
                packet._sendTime = clock::now();
                const auto& data = packet._packet->GetElement();
                const auto& sockaddr = packet._packet->GetDstAddr();
                size_t result = socket.SendTo(data, sockaddr.GetAddr());
                if (result != data.size())
                    throw std::runtime_error("sendto()");

            }

            if (!sendQueue.empty())
            {
                std::lock_guard lock(udpQueue._sendFreeQueue._mutex);
                udpQueue._sendFreeQueue.splice(udpQueue._sendFreeQueue.end(), sendQueue);
            }
        }

    }
}