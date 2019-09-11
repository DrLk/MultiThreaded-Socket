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

            src.OnSend = [&dst](BufferOwner::Ptr& packet)
            {
                dst.OnReceive(packet);
            };

            dst.OnSend = [&src](BufferOwner::Ptr& packet)
            {
                src.OnReceive(packet);
            };

            std::vector<char> data(100);
            IConnection* srcConnection = src.Connect(dstAddr);
            srcConnection->Send(data);


            BufferOwner::ElementType element2(1500);
            BufferOwner::Ptr synAckPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


            synAckPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synAckPacket->GetHeader().SetConnectionID(2);

            src.OnReceive(synAckPacket);



        }
    }
}
