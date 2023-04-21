#include "SendThreadQueue.hpp"

#include <thread>

#include "Socket.hpp"
#include "ThreadName.hpp"
#include "UDPQueue.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
SendThreadQueue::SendThreadQueue()
    : _pauseTime(1)
{
}

namespace {

    OutgoingPacket::List GetOutgoingPacketsToSend(std::stop_token stop, LockedList<OutgoingPacket>& sendQueue, size_t size)
    {
        OutgoingPacket::List outgoingPackets;
        {
            std::unique_lock lock(sendQueue._mutex);
            if (!sendQueue.Wait(lock, stop, [&sendQueue]() { return !sendQueue.empty(); })) {
                return outgoingPackets;
            }

            if (size < sendQueue.size()) {
                auto freeSendQueue = sendQueue.TryGenerate(size);
                outgoingPackets.splice(std::move(freeSendQueue));
            } else {
                outgoingPackets.swap(sendQueue);
            }
        }
        return outgoingPackets;
    }
} // namespace

void SendThreadQueue::WriteThread(std::stop_token stop, UDPQueue& udpQueue, SendThreadQueue& /*sendThreadQueue*/, const Socket& socket, size_t index)
{
    SetThreadName("WriteThread");

    OutgoingPacket::List sendQueue;

    while (!stop.stop_requested()) {

        if (sendQueue.empty()) {
            sendQueue = GetOutgoingPacketsToSend(stop, udpQueue._sendQueue, udpQueue._sendQueueSizePerThread);
        }

        for (auto& packet : sendQueue) {
            packet._sendTime = clock::now();
            const auto& data = packet._packet->GetElement();
            ConnectionAddr sockaddr = packet._packet->GetDstAddr();
            sockaddr.SetPort(sockaddr.GetPort() + index);

            if (!socket.WaitWrite()) {
                break;
            }

            while (true) {
                const int result = socket.SendTo(data, sockaddr.GetAddr());
                // WSAEWOULDBLOCK
                if (result == data.size()) {
                    break;
                }
            }
        }

        if (!sendQueue.empty()) {
            OutgoingPacket::List queue;
            queue.swap(sendQueue);
            const std::scoped_lock lock(udpQueue._sendFreeQueue._mutex);
            udpQueue._sendFreeQueue.splice(std::move(queue));
            udpQueue._sendFreeQueue.NotifyAll();
        }
    }
}
} // namespace FastTransport::Protocol
