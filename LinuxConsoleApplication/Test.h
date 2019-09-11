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

            BufferOwner::ElementType element(1500);
            BufferOwner::BufferType freeElements;
            BufferOwner::Ptr synPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(1);

            dst.OnReceive(synPacket);

            BufferOwner::ElementType element2(1500);
            BufferOwner::Ptr synAckPacket = std::make_shared<BufferOwner>(freeElements, std::move(element));


            synAckPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synAckPacket->GetHeader().SetConnectionID(2);

            src.OnReceive(synAckPacket);



        }
    }
}
