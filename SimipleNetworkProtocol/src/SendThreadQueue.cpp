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

    bool sleep = false;

    while (true) {
        if (sleep) {
            std::this_thread::sleep_for(1ms);
        }

        if (sendQueue.empty()) { // NOLINT
            const std::lock_guard lock(udpQueue._sendQueue._mutex);
            if (udpQueue._sendQueue.empty()) {
                sleep = true;
                continue;
            }
            sleep = false;

            if (udpQueue._sendQueueSizePerThread < udpQueue._sendQueue.size()) {
                auto freeSendQueue = udpQueue._sendQueue.TryGenerate(udpQueue._sendQueueSizePerThread);
                sendQueue.splice(std::move(freeSendQueue));
            } else {
                sendQueue = std::move(udpQueue._sendQueue);
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
            udpQueue._sendFreeQueue.splice(std::move(sendQueue)); // NOLINT
        }
    }
}
} // namespace FastTransport::Protocol
