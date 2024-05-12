#include "UDPQueue.hpp"

#include <Tracy.hpp>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <stop_token>
#include <utility>
#include <vector>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "Packet.hpp"
#include "RecvThreadQueue.hpp"
#include "SendThreadQueue.hpp"
#include "Socket.hpp"
#include "ThreadName.hpp"
#include "UDPQueueEvents.hpp"

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
    for (size_t i = 0; i < _threadCount; i++) {
        threadAddress.SetPort(_address.GetPort() + i);
        _sockets.emplace_back(std::make_shared<Socket>(threadAddress));
    }

    std::for_each(_sockets.begin(), _sockets.end(), [](std::shared_ptr<Socket>& socket) {
        socket->Init();
    });

    for (size_t i = 0; i < _threadCount; i++) {
        _sendThreadQueues.push_back(std::make_shared<SendThreadQueue>());
        _writeThreads.emplace_back(SendThreadQueue::WriteThread, std::ref(*this), std::ref(*_sendThreadQueues.back()), std::ref(*_sockets[i]), i);
    }

    for (size_t i = 0; i < _threadCount; i++) {
        _recvThreadQueues.push_back(std::make_shared<RecvThreadQueue>());
        _readThreads.emplace_back(ReadThread, std::ref(*this), std::ref(*_recvThreadQueues.back()), std::ref(*_sockets[i]), i);
    }
}

IPacket::List UDPQueue::Recv(std::stop_token stop, IPacket::List&& freeBuffers)
{
    if (!freeBuffers.empty()) {
        _recvFreeQueue.LockedSplice(std::move(freeBuffers));
        _recvFreeQueue.NotifyAll();
    }

    IPacket::List result;
    {
        _recvQueue.WaitFor(stop);
        _recvQueue.LockedSwap(result);
    }

    return result;
}

OutgoingPacket::List UDPQueue::Send(std::stop_token  /*stop*/, OutgoingPacket::List&& data)
{
    if (!data.empty()) {
        _sendQueue.LockedSplice(std::move(data));
        _sendQueue.NotifyAll();
    }

    OutgoingPacket::List result;
    {
        _sendFreeQueue.LockedSwap(result);
    }

    return result;
}

size_t UDPQueue::GetRecvQueueSizePerThread() const
{
    return _recvQueueSizePerThread;
}

size_t UDPQueue::GetThreadCount() const
{
    return _threadCount;
}

IPacket::List UDPQueue::CreateBuffers(size_t size)
{
    IPacket::List buffers;
    for (size_t i = 0; i < size; i++) {
        buffers.push_back(std::make_unique<Packet>(Socket::GsoSize));
    }

    return buffers;
}

void UDPQueue::Subscribe(UDPQueueEvents& events)
{
    _eventSubscribers.emplace_back(events);
}

void UDPQueue::ReadThread(std::stop_token stop, UDPQueue& udpQueue, RecvThreadQueue& recvThreadQueue, const Socket& socket, size_t index)
{
    SetThreadName("ReadThread");
    ZoneScopedN("UDPQueue::ReadThread");

    IPacket::List recvQueue;

    while (!stop.stop_requested()) {

        if (recvQueue.size() < Socket::UDPMaxSegments) {
            ZoneScopedN("RecvQueueEmpty");

            if (!udpQueue._recvFreeQueue.Wait(stop)) {
                continue;
            }

            recvQueue.splice(udpQueue._recvFreeQueue.LockedTryGenerate(udpQueue._recvQueueSizePerThread));
            continue;
        }

        if (!socket.WaitRead()) {
            continue;
        }

#ifdef __linux__

        IPacket::List freePackets = socket.RecvMsg(recvQueue, index);
        recvThreadQueue._recvThreadQueue.splice(std::move(recvQueue));
        recvQueue = std::move(freePackets);
#else

        for (auto it = recvQueue.begin(); it != recvQueue.end();) {
            ZoneScopedN("RecvQueueLoop");
            IPacket::Ptr& packet = *it;
            ConnectionAddr connectionAddr;
            const int result = socket.RecvFrom(packet->GetElement(), connectionAddr);
            // WSAEWOULDBLOCK
            if (result <= 0) {
                break;
            }

            connectionAddr.SetPort(connectionAddr.GetPort() - index);
            packet->SetAddr(connectionAddr);

            IPacket::List::Iterator temp = it;
            {
                recvThreadQueue._recvThreadQueue.push_back(std::move(*temp));
                it = recvQueue.erase(temp);
            }
        }

#endif

        if (!recvThreadQueue._recvThreadQueue.empty()) {
            ZoneScopedN("RecvThreadQueueEmpty");
            IPacket::List queue;
            queue.swap(recvThreadQueue._recvThreadQueue);
            udpQueue._recvQueue.LockedSplice(std::move(queue));
            udpQueue._recvQueue.NotifyAll();
        }
    }
}

void UDPQueue::NotifyOutgoingPacketsSubscribers()
{
    for (auto& subscriber : _eventSubscribers) {
        subscriber.get().OnOutgoingPackets();
    }
}
} // namespace FastTransport::Protocol
