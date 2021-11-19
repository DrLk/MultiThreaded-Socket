#pragma once

#include "IPacket.h"
#include "FastTransportProtocol.h"

namespace FastTransport
{
    namespace Protocol
    {
        void TestConnection()
        {
            ConnectionAddr addr("127.0.0.1", 8);
            ConnectionAddr addr2("127.0.0.2", 8);
            FastTransportContext src;
            FastTransportContext dst;
            ConnectionAddr srcAddr;
            ConnectionAddr dstAddr;

            BufferOwner::ElementType element(1500);
            BufferOwner::BufferType freeElements;

            src.OnSend = [&dst](std::list<OutgoingPacket>& packets)
            {
                std::list<std::unique_ptr<IPacket>> buffers;

                for (auto& packet : packets)
                {
                    packet._sendTime == Time::now();
                    buffers.push_back(std::move(packet._packet));
                }
                dst.OnReceive(std::move(buffers));
            };

            dst.OnSend = [&src](std::list<OutgoingPacket>& packets)
            {
                std::list<std::unique_ptr<IPacket>> buffers;

                for (auto& packet : packets)
                {
                    packet._sendTime == Time::now();
                    buffers.push_back(std::move(packet._packet));
                }

                src.OnReceive(std::move(buffers));
            };

            std::unique_ptr<IPacket> data = std::make_unique<BufferOwner>(1500);
            data->GetPayload();
            IConnection* srcConnection = src.Connect(dstAddr);

            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();
            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();

            srcConnection->Send(std::move(data));

            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();
            src.ConnectionsRun();
            src.SendQueueStep();
            dst.ConnectionsRun();
            dst.SendQueueStep();


            int a = 0;
            a++;

            BufferOwner::ElementType element2(1500);
            BufferOwner::Ptr synAckPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


        }
    }
}
