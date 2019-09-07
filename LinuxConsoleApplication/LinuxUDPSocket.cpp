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

LinuxUDPSocket::LinuxUDPSocket() : _socket(0), _port(8888), _sendQueueSizePerThread(1000), _recvQueueSizePerThread(1000)
{
    _writeThreadCount = 1;
    _readThreadCount = 1;
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
    if (bind(_socket, (struct sockaddr*) & si_me, sizeof(si_me)) == -1)
    {
        die("bind");
    }

    //keep listening for data
    /*while (1)
    {
        printf("Waiting for data...");
        fflush(stdout);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(_socket, buf, BUFLEN, 0, (struct sockaddr*) & si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

        //print details of the client/peer and the data received
        //printf(&quot; Received packet from % s: % d\n & quot; , inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        //printf(&quot; Data: % s\n & quot;, buf);

        //now reply the client with the same data
        if (sendto(_socket, buf, recv_len, 0, (struct sockaddr*) & si_other, slen) == -1)
        {
            die("sendto()");
        }
    }*/

    

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

std::list<Packet*>&& LinuxUDPSocket::Recv(std::list<Packet*>&& freeBuffers)
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
    return std::move(result);
}

std::list<Packet*>&& LinuxUDPSocket::Send(std::list<Packet*>&& data)
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

    return std::move(result);
}

void LinuxUDPSocket::WriteThread(LinuxUDPSocket& socket)
{
    std::list<Packet*> sendQueue;

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(socket._sendQueueMutex);

            if (socket._sendQueue.empty())
                usleep(1);

            if (sendQueue.empty())
            {
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
        }

        for (Packet* packet : sendQueue)
        {
            size_t result = sendto(socket._socket, packet->GetData().data(), packet->GetData().size(), 0, &(packet->GetAddr()), sizeof(packet->GetAddr()));
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

    while (true)
    {
        if (recvFreeQueue.empty())
        {
            std::lock_guard<std::mutex> lock(socket._recvFreeQueueMutex);

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
            packet->GetAddr() = addr;
            if (result < 0)
                throw std::runtime_error("sendto()");

            {
                std::lock_guard<std::mutex> lock(recvThreadQueue._recvThreadQueueMutex);
                recvThreadQueue._recvThreadQueue.splice(recvThreadQueue._recvThreadQueue.begin(), recvFreeQueue, it, it++);
            }

        }

        std::list<Packet*> packets;
        {
            std::lock_guard<std::mutex> lock2(recvThreadQueue._recvThreadQueueMutex);
            packets = std::move(recvThreadQueue._recvThreadQueue);
        }

        {
            std::lock_guard<std::mutex> lock(socket._recvQueueMutex);
            socket._recvQueue.splice(socket._recvQueue.begin(), std::move(packets));
        }
    }
}
