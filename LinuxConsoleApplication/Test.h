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

            src.OnSend = [&dst](std::list<BufferOwner::Ptr>&& packets)
            {
                dst.OnReceive(std::move(packets));
            };

            dst.OnSend = [&src](std::list<BufferOwner::Ptr>&& packets)
            {
                src.OnReceive(std::move(packets));
            };

            std::vector<char> data(100);
            IConnection* srcConnection = src.Connect(dstAddr);

            src.SendQueueStep();
            srcConnection->Send(data);



            BufferOwner::ElementType element2(1500);
            BufferOwner::Ptr synAckPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


            synAckPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synAckPacket->GetHeader().SetConnectionID(2);

            src.OnReceive(synAckPacket);



        }
    }
}
