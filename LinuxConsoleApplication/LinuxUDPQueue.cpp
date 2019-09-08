#include"LinuxUDPQueue.h"

#include <exception>
#include <algorithm>

#include "Packet.h"

#define BUFLEN 512	//Max length of buffer


void die(char* s)
{
    throw std::runtime_error(s);
}

LinuxUDPQueue::LinuxUDPQueue(int port, int sendThreadCount, int recvThreadCount, int sendQueueSizePerThread, int recvQueueSizePerThread) : _port(port)
{
    _writeThreadCount = sendThreadCount;
    _readThreadCount = recvThreadCount;
    _sendQueueSizePerThread = sendQueueSizePerThread;
    _recvQueueSizePerThread = recvQueueSizePerThread;
}

LinuxUDPQueue::~LinuxUDPQueue()
{
}

void LinuxUDPQueue::Init()
{
    for (int i = 0; i < std::max(_writeThreadCount, _readThreadCount); i++)
    {
        _sockets.emplace_back(new Socket(_port + i));
    }

    std::for_each(_sockets.begin(), _sockets.end(), [](Socket* socket) { socket->Init(); });

    for (int i = 0; i < _writeThreadCount; i++)
    {
        _writeThreads.push_back(std::thread(WriteThread, std::ref(*this), std::ref(*_sockets[i]), i));
    }

    for (int i = 0; i < _readThreadCount; i++)
    {
        _recvThreadQueues.emplace_back(new RecvThreadQueue());
        _readThreads.push_back(std::thread(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back()), std::ref(*_sockets[i]), i));
    }
}

std::list<Packet*> LinuxUDPQueue::Recv(std::list<Packet*>&& freeBuffers)
{
    {
        std::lock_guard<std::mutex> lock(_recvFreeQueueMutex);
        _recvFreeQueue.splice(_recvFreeQueue.end(), std::move(freeBuffers));
    }

    for (RecvThreadQueue* recvThreadQueue : _recvThreadQueues)
    {
        std::list<Packet*> packets;
        {
            std::lock_guard<std::mutex> lock(recvThreadQueue->_recvThreadQueueMutex);
            packets = std::move(recvThreadQueue->_recvThreadQueue);
        }

        {
            std::lock_guard<std::mutex> lock(_recvQueueMutex);
            _recvQueue.splice(_recvQueue.end(), std::move(packets));
        }
    }

    std::list<Packet*> result;
    {
        std::lock_guard<std::mutex> lock(_recvQueueMutex);
        result = std::move(_recvQueue);
    }
    return result;
}

std::list<Packet*> LinuxUDPQueue::Send(std::list<Packet*>&& data)
{
    {
        std::lock_guard<std::mutex> lock(_sendQueueMutex);
        _sendQueue.splice(_sendQueue.end(), std::move(data));
    }

    std::list<Packet*> result;
    {
        std::lock_guard<std::mutex> lock(_sendFreeQueueMutex);
        result = std::move(_sendFreeQueue);
    }

    return result;
}


std::list<Packet*> LinuxUDPQueue::CreateBuffers(int size)
{
    std::list<Packet*> buffers;
    for (int i = 0; i < size; i++)
    {
        buffers.emplace_back(new Packet());
    }

    return buffers;
}

void LinuxUDPQueue::WriteThread(LinuxUDPQueue& udpQueue, const Socket& socket, int index)
{
    std::list<Packet*> sendQueue;

    bool sleep = false;

    while (true)
    {
        if (sleep)
            usleep(1);

        if (sendQueue.empty())
        {
            std::lock_guard<std::mutex> lock(udpQueue._sendQueueMutex);
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
                for (int i = 0; i < udpQueue._sendQueueSizePerThread; i++)
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

        for (Packet* packet : sendQueue)
        {
            packet->GetAddr().sin_port = htons(ntohs(packet->GetAddr().sin_port) + index);
            size_t result = socket.SendTo(packet->GetData(), (sockaddr*)&(packet->GetAddr()), sizeof(sockaddr));
            //size_t result = sendto(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, packet->GetAddr(), sizeof(sockaddr));
            if (result != packet->GetData().size())
                throw std::runtime_error("sendto()");
        }

        if (!sendQueue.empty())
        {
            std::lock_guard<std::mutex> lock(udpQueue._sendFreeQueueMutex);
            udpQueue._sendFreeQueue.splice(udpQueue._sendFreeQueue.end(), std::move(sendQueue));
        }
    }

}

void LinuxUDPQueue::ReadThread(LinuxUDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, int index)
{
    std::list<Packet*> recvFreeQueue;
    bool sleep = false;

    while (true)
    {
        if (sleep)
            usleep(1);

        if (recvFreeQueue.empty())
        {
            std::lock_guard<std::mutex> lock(udpQueue._recvFreeQueueMutex);

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
                for (int i = 0; i < udpQueue._recvQueueSizePerThread; i++)
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
            usleep(1);

        for (auto it = recvFreeQueue.begin(); it != recvFreeQueue.end(); )
        {
            Packet* packet = *it;
            socklen_t len = sizeof(sockaddr_in);;
            //size_t result = recvfrom(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, &addr, &len);
            size_t result = socket.RecvFrom(packet->GetData(), (sockaddr*)&packet->GetAddr(), &len);
            packet->GetData().resize(result);
            packet->GetAddr().sin_port = htons(ntohs(packet->GetAddr().sin_port) - index);
            if (result <= 0)
                throw std::runtime_error("sendto()");

            auto temp = it;
            it++;
            {
                std::lock_guard<std::mutex> lock(recvThreadQueue._recvThreadQueueMutex);
                recvThreadQueue._recvThreadQueue.splice(recvThreadQueue._recvThreadQueue.begin(), recvFreeQueue, temp);
            }

        }

        std::list<Packet*> packets;
        {
            std::lock_guard<std::mutex> lock(recvThreadQueue._recvThreadQueueMutex);
            packets = std::move(recvThreadQueue._recvThreadQueue);
        }

        {
            std::lock_guard<std::mutex> lock(udpQueue._recvQueueMutex);
            udpQueue._recvQueue.splice(udpQueue._recvQueue.begin(), std::move(packets));
        }
    }
}
