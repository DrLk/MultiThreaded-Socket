#pragma once

#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>

#include "RecvThreadQueue.h"
#include "SendThreadQueue.h"
#include "Socket.h"
#include "LockedList.h"
#include "IPacket.h"

namespace FastTransport::Protocol
{
    class UDPQueue
    {
        friend SendThreadQueue;

    public:
        UDPQueue(int port, int threadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
        ~UDPQueue();
        void Init();

        std::list<std::unique_ptr<IPacket>> Recv(std::list<std::unique_ptr<IPacket>>&& freeBuffers);
        std::list<std::unique_ptr<IPacket>> Send(std::list<std::unique_ptr<IPacket>>&& data);

        std::list<std::unique_ptr<IPacket>> CreateBuffers(int size);

        std::function<void(const std::list<std::unique_ptr<IPacket>>& packets)> OnSend;
        std::function<void(std::list<std::unique_ptr<IPacket>>& packets)> OnRecv;

    private:
        unsigned int _port;
        std::vector<std::thread> _writeThreads;
        std::vector<std::thread> _readThreads;

        LockedList<std::unique_ptr<IPacket>> _sendQueue;
        LockedList<std::unique_ptr<IPacket>> _recvQueue;

        LockedList<std::unique_ptr<IPacket>> _sendFreeQueue;
        LockedList<std::unique_ptr<IPacket>> _recvFreeQueue;

        static void ReadThread(UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, unsigned short index);

        std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;
        std::vector<std::shared_ptr<SendThreadQueue>> _sendThreadQueues;

        int _threadCount;

        size_t _sendQueueSizePerThread;
        size_t _recvQueueSizePerThread;

        std::vector<std::shared_ptr<Socket>> _sockets;
    };
}
