#pragma once

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "IPacket.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"
#include "RecvThreadQueue.hpp"
#include "SendThreadQueue.hpp"
#include "Socket.hpp"

namespace FastTransport::Protocol {

using FastTransport::Containers::LockedList;

class UDPQueue {
    friend SendThreadQueue;

public:
    UDPQueue(int port, int threadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
    void Init();

    IPacket::List Recv(IPacket::List&& freeBuffers);
    OutgoingPacket::List Send(OutgoingPacket::List&& data);

    static IPacket::List CreateBuffers(int size);

private:
    unsigned int _port;
    std::vector<std::jthread> _writeThreads;
    std::vector<std::jthread> _readThreads;

    LockedList<OutgoingPacket> _sendQueue;
    LockedList<IPacket::Ptr> _recvQueue;

    LockedList<OutgoingPacket> _sendFreeQueue;
    LockedList<IPacket::Ptr> _recvFreeQueue;

    static void ReadThread(std::stop_token stop, UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, uint16_t index);

    std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;
    std::vector<std::shared_ptr<SendThreadQueue>> _sendThreadQueues;

    int _threadCount;

    size_t _sendQueueSizePerThread;
    size_t _recvQueueSizePerThread;

    std::vector<std::shared_ptr<Socket>> _sockets;
};
} // namespace FastTransport::Protocol
