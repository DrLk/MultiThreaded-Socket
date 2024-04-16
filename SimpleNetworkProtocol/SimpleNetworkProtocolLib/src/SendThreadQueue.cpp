#include "SendThreadQueue.hpp"

#include <Tracy.hpp>
#include <chrono>
#include <cstddef>
#include <stop_token>
#include <utility>

#ifndef __linux__
#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
#endif

#include "OutgoingPacket.hpp"
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
            if (!sendQueue.Wait(stop)) {
                return outgoingPackets;
            }

            outgoingPackets.splice(sendQueue.LockedTryGenerate(size));
        }
        return outgoingPackets;
    }
} // namespace

void SendThreadQueue::WriteThread(std::stop_token stop, UDPQueue& udpQueue, SendThreadQueue& /*sendThreadQueue*/, const Socket& socket, size_t index)
{
    SetThreadName("WriteThread");
    ZoneScopedN("UDPQueue::WriteThread");

    OutgoingPacket::List sendQueue;

    while (!stop.stop_requested()) {
        if (sendQueue.empty()) {
            ZoneScopedN("GetOutgoingPacketsToSend");
            sendQueue = GetOutgoingPacketsToSend(stop, udpQueue._sendQueue, udpQueue._sendQueueSizePerThread);
            continue;
        }

#ifdef __linux__
        OutgoingPacket::List list;
        sendQueue.swap(list);
        while (!list.empty()) {
            if (!socket.WaitWrite()) {
                break;
            }

            auto packets = list.TryGenerate(Socket::UDPMaxSegments);
            for (auto& packet : packets) {
                ZoneScopedN("SendQueueLoop");
                packet.SetSendTime(clock::now());
            }
            auto result = socket.SendMsg(packets, index);
            assert(result == packets.size() * Socket::GsoSize);
            sendQueue.splice(std::move(packets));
        }
        /* for (auto& packet : sendQueue) { */
        /*     ZoneScopedN("SendQueueLoop"); */
        /*     packet.SetSendTime(clock::now()); */
        /*     OutgoingPacket::List p1; */
        /*     p1.push_back(std::move(packet)); */
        /*     auto result = socket.SendMsg(p1, index); */
        /*     packet = std::move(p1.front()); */
        /* } */

        /* auto result = socket.SendMsg(sendQueue, index); */
#else
        for (auto& packet : sendQueue) {
            ZoneScopedN("SendQueueLoop");
            packet.SetSendTime(clock::now());
            auto data = packet.GetPacket()->GetElement();
            ConnectionAddr sockaddr = packet.GetPacket()->GetDstAddr();
            sockaddr.SetPort(sockaddr.GetPort() + index);

            if (!socket.WaitWrite()) {
                break;
            }

            while (true) {
                ZoneScopedN("SendPacket");
                const int result = socket.SendTo(data, sockaddr);
                // WSAEWOULDBLOCK
                if (result == data.size()) {
                    break;
                }
            }
        }
#endif

        if (!sendQueue.empty()) {
            ZoneScopedN("SendQueueEmpty");
            OutgoingPacket::List queue;
            queue.swap(sendQueue);
            udpQueue._sendFreeQueue.LockedSplice(std::move(queue));
            udpQueue._sendFreeQueue.NotifyAll();
        }
    }
}
} // namespace FastTransport::Protocol
