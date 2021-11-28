#include"LinuxUDPQueue.h"

#include <exception>
#include <algorithm>
#include <memory>

#include "Packet.h"

#define BUFLEN 512	//Max length of buffer


namespace FastTransport::Protocol
{
    void die(char* s)
    {
        throw std::runtime_error(s);
    }

    LinuxUDPQueue::LinuxUDPQueue(int port, int threadCount, int sendQueueSizePerThread, int recvQueueSizePerThread) : 
        _port(port), _threadCount(threadCount), _sendQueueSizePerThread(sendQueueSizePerThread), _recvQueueSizePerThread(recvQueueSizePerThread)
    {
    }

    LinuxUDPQueue::~LinuxUDPQueue()
    {
    }

    void LinuxUDPQueue::Init()
    {
        for (int i = 0; i < _threadCount; i++)
        {
            _sockets.emplace_back(std::make_shared<Socket>(_port + i));
        }

        std::for_each(_sockets.begin(), _sockets.end(), [](std::shared_ptr<Socket>& socket)
            {
                socket->Init();
            });

        for (int i = 0; i < _threadCount; i++)
        {
            _writeThreads.push_back(std::thread(WriteThread, std::ref(*this), std::ref(*_sockets[i]), i));
            _recvThreadQueues.emplace_back(std::make_shared<RecvThreadQueue>());
            _readThreads.push_back(std::thread(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back()), std::ref(*_sockets[i]), i));
        }
    }

    std::list<std::unique_ptr<IPacket>> LinuxUDPQueue::Recv(std::list<std::unique_ptr<IPacket>>&& freeBuffers)
    {
        {
            std::lock_guard<std::mutex> lock(_recvFreeQueue._mutex);
            _recvFreeQueue.splice(_recvFreeQueue.end(), std::move(freeBuffers));
        }

        std::list<std::unique_ptr<IPacket>> result;
        {
            std::lock_guard<std::mutex> lock(_recvQueue._mutex);
            result = std::move(_recvQueue);
        }
        return result;
    }

    std::list<std::unique_ptr<IPacket>> LinuxUDPQueue::Send(std::list<std::unique_ptr<IPacket>>&& data)
    {
        {
            std::lock_guard<std::mutex> lock(_sendQueue._mutex);
            _sendQueue.splice(_sendQueue.end(), std::move(data));
        }

        std::list<std::unique_ptr<IPacket>> result;
        {
            std::lock_guard<std::mutex> lock(_sendFreeQueue._mutex);
            result = std::move(_sendFreeQueue);
        }

        return result;
    }


    std::list<std::unique_ptr<IPacket>> LinuxUDPQueue::CreateBuffers(int size)
    {
        std::list<std::unique_ptr<IPacket>> buffers;
        for (int i = 0; i < size; i++)
        {
            buffers.emplace_back(new Packet(1500));
        }

        return buffers;
    }

    void LinuxUDPQueue::WriteThread(LinuxUDPQueue& udpQueue, const Socket& socket, unsigned short index)
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

            udpQueue.OnSend(sendQueue);

            if (!sendQueue.empty())
            {
                std::lock_guard<std::mutex> lock(udpQueue._sendFreeQueue._mutex);
                udpQueue._sendFreeQueue.splice(udpQueue._sendFreeQueue.end(), std::move(sendQueue));
            }
        }

    }

    void LinuxUDPQueue::ReadThread(LinuxUDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, unsigned short index)
    {
        std::list<std::unique_ptr<IPacket>> recvFreeQueue;
        bool sleep = false;

        while (true)
        {
            if (sleep)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (recvFreeQueue.empty())
            {
                std::lock_guard<std::mutex> lock(udpQueue._recvFreeQueue._mutex);

                if (udpQueue._recvFreeQueue.empty())
                {
                    sleep = true;
                    continue;
                }
                else
                {
                    sleep = false;
                }

                auto end = udpQueue._recvFreeQueue.begin();
                if (udpQueue._recvFreeQueue.size() > udpQueue._recvQueueSizePerThread)
                {
                    for (size_t i = 0; i < udpQueue._recvQueueSizePerThread; i++)
                    {
                        end++;
                    }
                    recvFreeQueue.splice(recvFreeQueue.begin(), udpQueue._recvFreeQueue, udpQueue._recvFreeQueue.begin(), end);
                }
                else
                {
                    recvFreeQueue.splice(recvFreeQueue.begin(), std::move(udpQueue._recvFreeQueue));
                }
            }

            if (recvFreeQueue.empty())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            for (auto it = recvFreeQueue.begin(); it != recvFreeQueue.end(); )
            {
                const std::unique_ptr<IPacket>& packet = *it;
                socklen_t len = sizeof(sockaddr_in);;
                //size_t result = recvfrom(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, &addr, &len);
                /*size_t result = socket.RecvFrom(packet->GetPayload(), (sockaddr*)&packet->GetAddr(), &len);
                packet->GetPayload().resize(result);
                packet->GetAddr().sin_port = htons((unsigned short)(ntohs(packet->GetAddr().sin_port) - index));
                if (result <= 0)
                    throw std::runtime_error("sendto()");*/

                auto temp = it;
                it++;
                {
                    recvThreadQueue._recvThreadQueue.splice(recvThreadQueue._recvThreadQueue.begin(), recvFreeQueue, temp);
                }

            }

            {
                std::lock_guard<std::mutex> lock(udpQueue._recvQueue._mutex);
                udpQueue._recvQueue.splice(udpQueue._recvQueue.begin(), std::move(recvThreadQueue._recvThreadQueue));
            }
        }
    }
}
