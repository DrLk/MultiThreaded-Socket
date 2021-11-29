#include "SendThreadQueue.h"

#include <thread>

#include "LinuxUDPQueue.h"
#include "IPacket.h"


namespace FastTransport::Protocol
{
    SendThreadQueue::SendThreadQueue() : _pauseTime(1)
    {
    }


    void SendThreadQueue::WriteThread(LinuxUDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, unsigned short index)
    {
        std::list<std::unique_ptr<IPacket>> sendQueue;

        bool sleep = false;

        while (true)
        {
            if (sleep)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (sendQueue.empty())
            {
                std::lock_guard<std::mutex> lock(udpQueue._sendQueue._mutex);
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
                    sendQueue = std::move(udpQueue._sendQueue);
                }
            }

            for (const auto& packet : sendQueue)
            {
                unsigned short currentPort = (unsigned short)(ntohs(packet->GetDstAddr().GetPort()) + (unsigned short)index);
                /*packet->GetDstAddr().GetPort() = htons(currentPort);
                size_t result = socket.SendTo(packet->GetData(), (sockaddr*) & (packet->GetAddr()), sizeof(sockaddr));
                size_t result = sendto(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, packet->GetAddr(), sizeof(sockaddr));
                if (result != packet->GetPayload().size())
                    throw std::runtime_error("sendto()");
                    */

            }

            if (udpQueue.OnSend)
                udpQueue.OnSend(sendQueue);

            if (!sendQueue.empty())
            {
                std::lock_guard<std::mutex> lock(udpQueue._sendFreeQueue._mutex);
                udpQueue._sendFreeQueue.splice(udpQueue._sendFreeQueue.end(), std::move(sendQueue));
            }
        }

    }
}