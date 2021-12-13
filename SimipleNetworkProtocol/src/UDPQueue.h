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
#include "OutgoingPacket.h"

namespace FastTransport::Protocol
{
    class UDPQueue
    {
        friend SendThreadQueue;

    public:
        UDPQueue(int port, int threadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
        ~UDPQueue();
        void Init();

        std::list<IPacket::Ptr> Recv(std::list<IPacket::Ptr>&& freeBuffers);
        std::list<OutgoingPacket> Send(std::list<OutgoingPacket>&& data);

        std::list<IPacket::Ptr> CreateBuffers(int size);

    private:
        unsigned int _port;
        std::vector<std::thread> _writeThreads;
        std::vector<std::thread> _readThreads;

        LockedList<OutgoingPacket> _sendQueue;
        LockedList<IPacket::Ptr> _recvQueue;

        LockedList<OutgoingPacket> _sendFreeQueue;
        LockedList<IPacket::Ptr> _recvFreeQueue;

        static void ReadThread(UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, unsigned short index);

        std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;
        std::vector<std::shared_ptr<SendThreadQueue>> _sendThreadQueues;

        int _threadCount;

        size_t _sendQueueSizePerThread;
        size_t _recvQueueSizePerThread;

        std::vector<std::shared_ptr<Socket>> _sockets;
    };
}
