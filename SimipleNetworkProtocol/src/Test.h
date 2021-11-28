#pragma once

#include <iostream>

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

            Packet::ElementType element(1500);
            Packet::BufferType freeElements;

            src.OnSend = [&dst](std::list<OutgoingPacket>& packets)
            {
                std::list<std::unique_ptr<IPacket>> recvPackets;
                for (auto& packet : packets)
                {
                    packet._sendTime = Time::now();

                    std::unique_ptr<IPacket> rcvPacket = std::make_unique<Packet>(1500);
                    rcvPacket->Copy(*packet._packet);
                    recvPackets.push_back(std::move(rcvPacket));
                }

                dst.OnReceive(std::move(recvPackets));
            };

            dst.OnSend = [&src](std::list<OutgoingPacket>& packets)
            {
                std::list<std::unique_ptr<IPacket>> recvPackets;
                for (auto& packet : packets)
                {
                    packet._sendTime = Time::now();

                    std::unique_ptr<IPacket> rcvPacket = std::make_unique<Packet>(1500);
                    rcvPacket->Copy(*packet._packet);
                    recvPackets.push_back(std::move(rcvPacket));

                }

                src.OnReceive(std::move(recvPackets));
            };

            std::unique_ptr<IPacket> data = std::make_unique<Packet>(1500);
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

            Packet::ElementType element2(1500);
            Packet::Ptr synAckPacket = std::make_shared<Packet>(freeElements, std::move(element));


        }

        void TestTimer()
        {
            unsigned long long counter = 0;
            std::time_t end_time;
            ms elapsed_seconds;
            auto start = std::chrono::system_clock::now();
            // Some computation here
            while (true)
            {
                counter++;
                auto end = std::chrono::system_clock::now();

                elapsed_seconds = std::chrono::duration_cast<ms>(end - start);
                if (elapsed_seconds > ms(1000))
                    break;
                end_time = std::chrono::system_clock::to_time_t(end);
            }

            std::cout << "finished computation at " << std::ctime(&end_time)
                << "elapsed time: " << elapsed_seconds.count() << "ms\n" << "counter: " << counter << std::endl;

        }
    }
}
