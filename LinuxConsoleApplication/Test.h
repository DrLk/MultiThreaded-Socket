#pragma once

#include "BufferOwner.h"
#include "FastTransportProtocol.h"

namespace FastTransport
{
    namespace Protocol
    {
        void TestConnection()
        {

            FastTransportContext src;
            FastTransportContext dst;
            ConnectionAddr srcAddr;
            ConnectionAddr dstAddr;

            BufferOwner::ElementType element(1500);
            BufferOwner::BufferType freeElements;

            src.OnSend = [&dst](std::list<OutgoingPacket>& packets)
            {
                std::list<BufferOwner::Ptr> buffers;

                for (auto& packet : packets)
                {
                    packet._sendTime == Time::now();
                    buffers.push_back(packet._packet);
                }
                dst.OnReceive(std::move(buffers));
            };

            dst.OnSend = [&src](std::list<OutgoingPacket>& packets)
            {
                std::list<BufferOwner::Ptr> buffers;

                for (auto& packet : packets)
                {
                    packet._sendTime == Time::now();
                    buffers.push_back(packet._packet);
                }

                src.OnReceive(std::move(buffers));
            };

            std::vector<char> data(100);
            IConnection* srcConnection = src.Connect(dstAddr);

            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();
            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();

            srcConnection->Send(data);



            BufferOwner::ElementType element2(1500);
            BufferOwner::Ptr synAckPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


            synAckPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synAckPacket->GetHeader().SetConnectionID(2);

            src.OnReceive(synAckPacket);



        }
    }
}
