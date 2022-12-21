#include "SendThreadQueue.hpp"

#include <thread>

#include "UDPQueue.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {
SendThreadQueue::SendThreadQueue()
    : _pauseTime(1)
{
}

void SendThreadQueue::WriteThread(UDPQueue& udpQueue, SendThreadQueue& /*sendThreadQueue*/, const Socket& socket, uint16_t index)
{
    OutgoingPacket::List sendQueue;

    while (true) {
        if (sendQueue.empty()) { // NOLINT
            std::unique_lock lock(udpQueue._sendQueue._mutex);
            udpQueue._sendQueue._condition.wait(lock, [&udpQueue]() { return !udpQueue._sendQueue.empty(); });

            if (udpQueue._sendQueueSizePerThread < udpQueue._sendQueue.size()) {
                auto freeSendQueue = udpQueue._sendQueue.TryGenerate(udpQueue._sendQueueSizePerThread);
                sendQueue.splice(std::move(freeSendQueue));
            } else {
                LockedList<OutgoingPacket> queue;
                queue.swap(udpQueue._sendQueue);
                sendQueue = std::move(queue);
            }
        }

        for (auto& packet : sendQueue) {
            packet._sendTime = clock::now();
            const auto& data = packet._packet->GetElement();
            auto& sockaddr = packet._packet->GetDstAddr();
            sockaddr.SetPort(sockaddr.GetPort() + index);

            while (true) {
                socket.WaitWrite();
                const int result = socket.SendTo(data, sockaddr.GetAddr());
                // WSAEWOULDBLOCK
                if (result == data.size()) {
                    break;
                }
            }

            sockaddr.SetPort(sockaddr.GetPort() - index);
        }

        if (!sendQueue.empty()) {
            const std::lock_guard lock(udpQueue._sendFreeQueue._mutex);
            OutgoingPacket::List queue;
            queue.swap(sendQueue);
            udpQueue._sendFreeQueue.splice(std::move(queue)); // NOLINT
        }
    }
}
} // namespace FastTransport::Protocol
