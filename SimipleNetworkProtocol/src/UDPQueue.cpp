#include "UDPQueue.hpp"

#include <algorithm>
#include <exception>
#include <memory>

#include "Packet.hpp"
#include "ThreadName.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
UDPQueue::UDPQueue(const ConnectionAddr& address, int threadCount, int sendQueueSizePerThread, int recvQueueSizePerThread)
    : _address(address)
    , _threadCount(threadCount)
    , _sendQueueSizePerThread(sendQueueSizePerThread)
    , _recvQueueSizePerThread(recvQueueSizePerThread)
{
}

void UDPQueue::Init()
{
    ConnectionAddr threadAddress = _address;
    for (int i = 0; i < _threadCount; i++) {
        threadAddress.SetPort(_address.GetPort() + i);
        _sockets.emplace_back(std::make_shared<Socket>(threadAddress));
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

IPacket::List UDPQueue::Recv(std::stop_token stop, IPacket::List&& freeBuffers)
{
    {
        const std::scoped_lock lock(_recvFreeQueue._mutex);
        _recvFreeQueue.splice(std::move(freeBuffers));
    }
    _recvFreeQueue.NotifyAll();

    IPacket::List result;
    {
        std::unique_lock lock(_recvQueue._mutex);
        _recvQueue.WaitFor(lock, stop, [this]() { return !_recvQueue.empty(); });
        result.swap(_recvQueue);
    }

    return result;
}

OutgoingPacket::List UDPQueue::Send(std::stop_token stop, OutgoingPacket::List&& data)
{
    if (!data.empty()) {
        const std::scoped_lock lock(_sendQueue._mutex);
        _sendQueue.splice(std::move(data));
        _sendQueue.NotifyAll();
    }

    OutgoingPacket::List result;
    {
        std::unique_lock lock(_sendFreeQueue._mutex);
        _sendFreeQueue.WaitFor(lock, stop, [this]() { return !_sendFreeQueue.empty(); });
        result.swap(_sendFreeQueue);
    }

    return result;
}

IPacket::List UDPQueue::CreateBuffers(int size)
{
    IPacket::List buffers;
    for (int i = 0; i < size; i++) {
        buffers.push_back(std::make_unique<Packet>(1400));
    }

    return buffers;
}

void UDPQueue::ReadThread(std::stop_token stop, UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, uint16_t index)
{
    SetThreadName("ReadThread");

    IPacket::List recvQueue;

    while (!stop.stop_requested()) {

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

        if (!recvThreadQueue._recvThreadQueue.empty()) {
            IPacket::List queue;
            queue.swap(recvThreadQueue._recvThreadQueue);
            const std::scoped_lock lock(udpQueue._recvQueue._mutex);
            udpQueue._recvQueue.splice(std::move(queue));
            udpQueue._recvQueue.NotifyAll();
        }
    }
}
} // namespace FastTransport::Protocol
