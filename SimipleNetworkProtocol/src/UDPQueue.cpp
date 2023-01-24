#include "UDPQueue.hpp"

#include <algorithm>
#include <exception>
#include <memory>

#include "Packet.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
UDPQueue::UDPQueue(int port, int threadCount, int sendQueueSizePerThread, int recvQueueSizePerThread)
    : _port(port)
    , _threadCount(threadCount)
    , _sendQueueSizePerThread(sendQueueSizePerThread)
    , _recvQueueSizePerThread(recvQueueSizePerThread)
{
}

void UDPQueue::Init()
{
    for (int i = 0; i < _threadCount; i++) {
        _sockets.emplace_back(std::make_shared<Socket>(_port + i));
    }

    std::for_each(_sockets.begin(), _sockets.end(), [](std::shared_ptr<Socket>& socket) {
        socket->Init();
    });

    for (int i = 0; i < _threadCount; i++) {
        _sendThreadQueues.push_back(std::make_shared<SendThreadQueue>());
        _writeThreads.emplace_back(SendThreadQueue::WriteThread, std::ref(*this), std::ref(*_sendThreadQueues.back()), std::ref(*_sockets[i]), i);
        _recvThreadQueues.push_back(std::make_shared<RecvThreadQueue>());
        _readThreads.emplace_back(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back()), std::ref(*_sockets[i]), i);
    }
}

IPacket::List UDPQueue::Recv(IPacket::List&& freeBuffers)
{
    {
        const std::lock_guard lock(_recvFreeQueue._mutex);
        _recvFreeQueue.splice(std::move(freeBuffers));
    }
    _recvFreeQueue.NotifyAll();

    IPacket::List result;
    {
        const std::lock_guard lock(_recvQueue._mutex);
        result.swap(_recvQueue);
    }

    return result;
}

OutgoingPacket::List UDPQueue::Send(OutgoingPacket::List&& data)
{
    if (!data.empty()) {
        const std::lock_guard lock(_sendQueue._mutex);
        _sendQueue.splice(std::move(data));
        _sendQueue.NotifyAll();
    }

    OutgoingPacket::List result;
    {
        const std::lock_guard lock(_sendFreeQueue._mutex);
        result.swap(_sendFreeQueue);
    }

    return result;
}

IPacket::List UDPQueue::CreateBuffers(int size)
{
    IPacket::List buffers;
    for (int i = 0; i < size; i++) {
        buffers.push_back(IPacket::Ptr(new Packet(1500)));
    }

    return buffers;
}

void UDPQueue::ReadThread(std::stop_token stop, UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, uint16_t index)
{
    IPacket::List recvQueue;

    while (true) {

        if (stop.stop_requested()) {
            return;
        }

        if (recvQueue.empty()) {
            std::unique_lock lock(udpQueue._recvFreeQueue._mutex);

            if (!udpQueue._recvFreeQueue.Wait(lock, stop, [&udpQueue] { return !udpQueue._recvFreeQueue.empty(); })) {
                continue;
            }
            if (udpQueue._recvFreeQueue.size() > udpQueue._recvQueueSizePerThread) {
                auto freeRecvQueue = udpQueue._recvFreeQueue.TryGenerate(udpQueue._recvQueueSizePerThread);
                recvQueue.splice(std::move(freeRecvQueue));
            } else {
                LockedList<IPacket::Ptr> queue;
                queue.swap(udpQueue._recvFreeQueue);
                recvQueue.splice(std::move(queue));
            }
        }

        if (!socket.WaitRead()) {
            continue;
        }

        for (auto it = recvQueue.begin(); it != recvQueue.end();) {
            IPacket::Ptr& packet = *it;
            sockaddr_storage sockAddr {};
            const int result = socket.RecvFrom(packet->GetElement(), sockAddr);
            // WSAEWOULDBLOCK
            if (result <= 0) {
                break;
            }

            packet->GetElement().resize(result);
            ConnectionAddr connectionAddr(sockAddr);
            connectionAddr.SetPort(connectionAddr.GetPort() - index);
            packet->SetAddr(connectionAddr);

            IPacket::List::Iterator temp = it;
            ++it;
            {
                recvThreadQueue._recvThreadQueue.push_back(std::move(*temp));
                recvQueue.erase(temp);
            }
        }

        {
            IPacket::List queue;
            queue.swap(recvThreadQueue._recvThreadQueue);
            const std::lock_guard lock(udpQueue._recvQueue._mutex);
            udpQueue._recvQueue.splice(std::move(queue));
        }
    }
}
} // namespace FastTransport::Protocol
