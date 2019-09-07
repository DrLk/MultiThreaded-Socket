#include"LinuxUDPSocket.h"

#include<string.h> //memset
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <exception>

#include "Packet.h"

#define BUFLEN 512	//Max length of buffer


void die(char* s)
{
    throw std::runtime_error(s);
}

LinuxUDPSocket::LinuxUDPSocket(int port, int sendThreadCount, int recvThreadCount, int sendQueueSizePerThread, int recvQueueSizePerThread) : _socket(0), _port(port)
{
    _writeThreadCount = sendThreadCount;
    _readThreadCount = recvThreadCount;
    _sendQueueSizePerThread = sendQueueSizePerThread;
    _recvQueueSizePerThread = recvQueueSizePerThread;
}

LinuxUDPSocket::~LinuxUDPSocket()
{
    close(_socket);
}

void LinuxUDPSocket::Init()
{
    struct sockaddr_in si_me, si_other;

    uint i, slen = sizeof(si_other), recv_len;
    char buf[BUFLEN];

    //create a UDP socket
    if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    // zero out the structure
    memset((char*)& si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(_port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if (bind(_socket, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
    {
        die("bind");
    }

    for (int i = 0; i < _writeThreadCount; i++)
    {
        _writeThreads.push_back(std::thread(WriteThread, std::ref(*this)));
    }

    for (int i = 0; i < _readThreadCount; i++)
    {
        _recvThreadQueues.emplace_back(new RecvThreadQueue());
        _readThreads.push_back(std::thread(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back())));
    }
}

std::list<Packet*> LinuxUDPSocket::Recv(std::list<Packet*>&& freeBuffers)
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

std::list<Packet*> LinuxUDPSocket::Send(std::list<Packet*>&& data)
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


std::list<Packet*> LinuxUDPSocket::CreateBuffers(int size)
{
    std::list<Packet*> buffers;
    for (int i = 0; i < size; i++)
    {
        buffers.emplace_back(new Packet());
    }

    return buffers;
}

void LinuxUDPSocket::WriteThread(LinuxUDPSocket& socket)
{
    std::list<Packet*> sendQueue;

    bool sleep = false;

    while (true)
    {
        if (sleep)
            usleep(1);

        if (sendQueue.empty())
        {
            std::lock_guard<std::mutex> lock(socket._sendQueueMutex);
            if (socket._sendQueue.empty())
            {
                sleep = true;
                continue;
            }
            else
            {
                sleep = false;
            }

            if (socket._sendQueueSizePerThread < socket._sendQueue.size())
            {
                auto end = socket._sendQueue.begin();
                for (int i = 0; i < socket._sendQueueSizePerThread; i++)
                {
                    end++;
                }
                sendQueue.splice(sendQueue.begin(), socket._sendQueue, socket._sendQueue.begin(), end);
            }
            else
            {
                sendQueue = std::move(socket._sendQueue);
            }
        }

        for (Packet* packet : sendQueue)
        {
            size_t result = sendto(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, packet->GetAddr(), sizeof(sockaddr));
            if (result != packet->GetData().size())
                throw std::runtime_error("sendto()");
        }

        if (!sendQueue.empty())
        {
            std::lock_guard<std::mutex> lock(socket._sendFreeQueueMutex);
            socket._sendFreeQueue.splice(socket._sendFreeQueue.end(), std::move(sendQueue));
        }
    }

}

void LinuxUDPSocket::ReadThread(LinuxUDPSocket& socket, RecvThreadQueue& recvThreadQueue)
{
    std::list<Packet*> recvFreeQueue;
    bool sleep = false;

    while (true)
    {
        if (sleep)
            usleep(1);

        if (recvFreeQueue.empty())
        {
            std::lock_guard<std::mutex> lock(socket._recvFreeQueueMutex);

            if (socket._recvFreeQueue.empty())
            {
                sleep = true;
                continue;
            }
            else
            {
                sleep = false;
            }

            auto end = socket._recvFreeQueue.begin();
            if (socket._recvFreeQueue.size() > socket._recvQueueSizePerThread)
            {
                for (int i = 0; i < socket._recvQueueSizePerThread; i++)
                {
                    end++;
                }
                recvFreeQueue.splice(recvFreeQueue.begin(), socket._recvFreeQueue, socket._recvFreeQueue.begin(), end);
            }
            else
            {
                recvFreeQueue.splice(recvFreeQueue.begin(), std::move(socket._recvFreeQueue));
            }
        }

        if (recvFreeQueue.empty())
            usleep(1);

        for (auto it = recvFreeQueue.begin(); it != recvFreeQueue.end(); )
        {
            Packet* packet = *it;
            sockaddr addr;
            socklen_t len = 0;;
            size_t result = recvfrom(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, &addr, &len);
            packet->GetData().resize(result);
            packet->SetAddr(nullptr);
            //packet->GetAddr() = addr; // TODO: not implement
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
            std::lock_guard<std::mutex> lock(socket._recvQueueMutex);
            socket._recvQueue.splice(socket._recvQueue.begin(), std::move(packets));
        }
    }
}
