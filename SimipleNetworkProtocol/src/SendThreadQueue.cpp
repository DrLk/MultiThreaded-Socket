#include "SendThreadQueue.h"

#include <thread>

#include "UDPQueue.h"

using namespace std::chrono_literals;


namespace FastTransport::Protocol
{
    SendThreadQueue::SendThreadQueue() : _pauseTime(1)
    {
    }


    void SendThreadQueue::WriteThread(UDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, unsigned short index)
    {
        OutgoingPacket::List sendQueue;

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
                    auto freeSendQueue = udpQueue._sendQueue.TryGenerate(udpQueue._sendQueueSizePerThread);
                    sendQueue.splice(std::move(freeSendQueue));
                }
                else
                {
                    sendQueue = std::move(udpQueue._sendQueue);
                }
            }

            for (auto& packet : sendQueue)
            {
                unsigned short currentPort = (unsigned short)(ntohs(packet._packet->GetDstAddr().GetPort()) + (unsigned short)index);
                packet._sendTime = clock::now();
                const auto& data = packet._packet->GetElement();
                auto& sockaddr = packet._packet->GetDstAddr();
                sockaddr.SetPort(sockaddr.GetPort() + index);

                while (true)
                {
                    int result = socket.SendTo(data, sockaddr.GetAddr());
                    // WSAEWOULDBLOCK
                    if (result == data.size())
                        break;
                }

                sockaddr.SetPort(sockaddr.GetPort() - index);
            }

            if (!sendQueue.empty())
            {
                std::lock_guard lock(udpQueue._sendFreeQueue._mutex);
                udpQueue._sendFreeQueue.splice(std::move(sendQueue));
            }
        }

    }
}