#pragma once

#include<vector>
#include<list>
#include<thread>
#include<mutex>
#include<memory>

#include "RecvThreadQueue.h"
#include "Socket.h"
#include "LockedList.h"

namespace FastTransport::UDPQueue
{
    class Packet;

    class LinuxUDPQueue
    {
    public:
        LinuxUDPQueue(int port, int threadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
        ~LinuxUDPQueue();
        void Init();

        std::list<Packet*> Recv(std::list<Packet*>&& freeBuffers);
        std::list<Packet*> Send(std::list<Packet*>&& data);

        std::list<Packet*> CreateBuffers(int size);

    private:
        uint _port;
        std::vector<std::thread> _writeThreads;
        std::vector<std::thread> _readThreads;

        LockedList<Packet*> _sendQueue;
        LockedList<Packet*> _recvQueue;

        LockedList<Packet*> _sendFreeQueue;
        LockedList<Packet*> _recvFreeQueue;

        static void WriteThread(LinuxUDPQueue& udpQueue, const Socket& socket, unsigned short index);
        static void ReadThread(LinuxUDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, unsigned short index);

        std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;

        int _threadCount;

        size_t _sendQueueSizePerThread;
        size_t _recvQueueSizePerThread;

        std::vector<std::shared_ptr<Socket>> _sockets;
    };
}
