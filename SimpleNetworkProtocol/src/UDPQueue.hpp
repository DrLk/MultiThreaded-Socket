#pragma once

#include <memory>
#include <stddef.h>
#include <stop_token>
#include <thread>
#include <vector>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
#include "LockedList.hpp"
#include "OutgoingPacket.hpp"

namespace FastTransport {
namespace Protocol {
    class RecvThreadQueue;
    class SendThreadQueue;
} // namespace Protocol
} // namespace FastTransport

namespace FastTransport::Protocol {

using FastTransport::Containers::LockedList;

class Socket;

class UDPQueue {
    friend SendThreadQueue;

public:
    UDPQueue(const ConnectionAddr& address, int threadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
    void Init();

    IPacket::List Recv(std::stop_token stop, IPacket::List&& freeBuffers);
    OutgoingPacket::List Send(std::stop_token stop, OutgoingPacket::List&& data);

    [[nodiscard]] size_t GetRecvQueueSizePerThread() const;
    [[nodiscard]] size_t GetThreadCount() const;

    static IPacket::List CreateBuffers(size_t size);

private:
    ConnectionAddr _address;

    LockedList<OutgoingPacket> _sendQueue;
    LockedList<IPacket::Ptr> _recvQueue;

    LockedList<OutgoingPacket> _sendFreeQueue;
    LockedList<IPacket::Ptr> _recvFreeQueue;

    static void ReadThread(std::stop_token stop, UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, size_t index);

    std::vector<std::shared_ptr<RecvThreadQueue>> _recvThreadQueues;
    std::vector<std::shared_ptr<SendThreadQueue>> _sendThreadQueues;

    size_t _threadCount;

    size_t _sendQueueSizePerThread;
    size_t _recvQueueSizePerThread;

    std::vector<std::shared_ptr<Socket>> _sockets;

    std::vector<std::jthread> _writeThreads;
    std::vector<std::jthread> _readThreads;
};
} // namespace FastTransport::Protocol
