#include"UDPQueue.h"

#include <exception>
#include <algorithm>
#include <memory>

#include "Packet.h"

using namespace std::chrono_literals;

namespace FastTransport::Protocol
{
    void die(char* s)
    {
        throw std::runtime_error(s);
    }

    UDPQueue::UDPQueue(int port, int threadCount, int sendQueueSizePerThread, int recvQueueSizePerThread) : 
        _port(port), _threadCount(threadCount), _sendQueueSizePerThread(sendQueueSizePerThread), _recvQueueSizePerThread(recvQueueSizePerThread)
    {
    }

    UDPQueue::~UDPQueue()
    {
    }

    void UDPQueue::Init()
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
            _sendThreadQueues.push_back(std::make_shared<SendThreadQueue>());
            _writeThreads.push_back(std::thread(SendThreadQueue::WriteThread, std::ref(*this), std::ref(*_sendThreadQueues.back()), std::ref(*_sockets[i]), i));
            _recvThreadQueues.push_back(std::make_shared<RecvThreadQueue>());
            _readThreads.push_back(std::thread(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back()), std::ref(*_sockets[i]), i));
        }
    }

    IPacket::List UDPQueue::Recv(IPacket::List&& freeBuffers)
    {
        {
            std::lock_guard lock(_recvFreeQueue._mutex);
            _recvFreeQueue.splice(std::move(freeBuffers));
        }

        IPacket::List result;
        {
            std::lock_guard lock(_recvQueue._mutex);
            if(!_recvQueue.empty())
                result = std::move(_recvQueue);
        }
        return result;
    }

    OutgoingPacket::List UDPQueue::Send(OutgoingPacket::List&& data)
    {
        {
            std::lock_guard lock(_sendQueue._mutex);
            _sendQueue.splice(std::move(data));
        }

        OutgoingPacket::List result;
        {
            std::lock_guard lock(_sendFreeQueue._mutex);
            result = std::move(_sendFreeQueue);
        }

        return result;
    }


    IPacket::List UDPQueue::CreateBuffers(int size)
    {
        IPacket::List buffers;
        for (int i = 0; i < size; i++)
        {
            buffers.push_back(IPacket::Ptr(new Packet(1500)));
        }

        return buffers;
    }

    
    void UDPQueue::ReadThread(UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, unsigned short index)
    {
        IPacket::List recvQueue;
        bool sleep = false;

        while (true)
        {
            if (sleep)
                std::this_thread::sleep_for(1ms);

            if (recvQueue.empty())
            {
                std::lock_guard lock(udpQueue._recvFreeQueue._mutex);

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
                    auto freeRecvQueue = udpQueue._recvFreeQueue.TryGenerate(udpQueue._recvQueueSizePerThread);
                    recvQueue.splice(std::move(freeRecvQueue));
                }
                else
                {
                    recvQueue.splice(std::move(udpQueue._recvFreeQueue));
                }
            }

            if (recvQueue.empty())
                std::this_thread::sleep_for(1ms);

            for (auto it = recvQueue.begin(); it != recvQueue.end(); )
            {
                IPacket::Ptr& packet = *it;
                sockaddr_storage sockAddr;
                int result = socket.RecvFrom(packet->GetElement(), sockAddr);
                // WSAEWOULDBLOCK
                if (result <= 0)
                {
                    break;
                }

                packet->GetElement().resize(result);
                ConnectionAddr connectionAddr(sockAddr);
                connectionAddr.SetPort(connectionAddr.GetPort() - index);
                packet->SetAddr(connectionAddr);

                auto temp = it;
                it++;
                {
                    recvThreadQueue._recvThreadQueue.push_back(std::move(*temp));
                    recvQueue.erase(temp);
                }

            }

            {
                std::lock_guard lock(udpQueue._recvQueue._mutex);
                udpQueue._recvQueue.splice(std::move(recvThreadQueue._recvThreadQueue));
            }
        }
    }
}
