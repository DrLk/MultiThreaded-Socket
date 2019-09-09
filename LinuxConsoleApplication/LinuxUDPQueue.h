#pragma once

#include<vector>
#include<list>
#include<thread>
#include<mutex>
#include<memory>

#include "RecvThreadQueue.h"
#include "Socket.h"

namespace FastTransport
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

        std::list<Packet*> _sendQueue;
        std::list<Packet*> _recvQueue;

        std::mutex _sendQueueMutex;
        std::mutex _recvQueueMutex;

        std::list<Packet*> _sendFreeQueue;
        std::list<Packet*> _recvFreeQueue;

        std::mutex _sendFreeQueueMutex;
        std::mutex _recvFreeQueueMutex;

        static void WriteThread(LinuxUDPQueue& udpQueue, const Socket& socket, int index);
        static void ReadThread(LinuxUDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, int index);

        std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;

        int _threadCount;

        int _sendQueueSizePerThread;
        int _recvQueueSizePerThread;

        std::vector<std::shared_ptr<Socket>> _sockets;
    };
}
