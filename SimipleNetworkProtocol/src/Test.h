#pragma once

#include <iostream>
#include <thread>
#include <chrono>

#include "IPacket.h"
#include "FastTransportProtocol.h"
#include "PeriodicExecutor.h"

namespace FastTransport
{
    namespace Protocol
    {
        void TestConnection()
        {
            ConnectionAddr addr("127.0.0.1", 8);
            FastTransportContext src(10100);
            FastTransportContext dst(10200);
            ConnectionAddr srcAddr;
            ConnectionAddr dstAddr;

            src.OnSend = [&dst](std::list<OutgoingPacket>& packets)
            {
                std::list<std::unique_ptr<IPacket>> recvPackets;
                for (auto& packet : packets)
                {
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
                    packet._sendTime = std::chrono::steady_clock::now();

                    std::unique_ptr<IPacket> rcvPacket = std::make_unique<Packet>(1500);
                    rcvPacket->Copy(*packet._packet);
                    recvPackets.push_back(std::move(rcvPacket));

                }

                src.OnReceive(std::move(recvPackets));
            };

            std::unique_ptr<IPacket> data = std::make_unique<Packet>(1500);
            data->GetPayload();
            IConnection* srcConnection = src.Connect(dstAddr);

            srcConnection->Send(std::move(data));

            while (true)
            {
                IConnection* dstConnection;
                while (true)
                {
                    dstConnection = dst.Accept();

                    if (dstConnection)
                        break;

                    std::this_thread::sleep_for(500ms);
                }
                std::list <std::unique_ptr<IPacket>> recvPackets;
                std::unique_ptr<IPacket> recvPacket = std::make_unique<Packet>(1500);;
                recvPackets.push_back(std::move(recvPacket));
                auto data = dstConnection->Recv(std::move(recvPackets));
                if (!data.empty())
                {
                    int a = 0;
                    a++;
                }

                std::this_thread::sleep_for(500ms);
            }
        }

        void TestTimer()
        {
            unsigned long long counter = 0;
            typedef std::chrono::steady_clock clock;
            auto start = clock::now();
            // Some computation here
            while (true)
            {
                counter++;
                auto end = clock::now();

                auto elapsed_seconds = end - start;

                if (elapsed_seconds > 5000ms)
                    break;
            }

            std::cout << "counter: " << counter << std::endl;

        }


	/*
	 * Linux rpi4 aarch64 manjaro 13.12.2021 counter about 12k per second per thread.
	 * 10 threads perform over 110k counter per seconds
	 * 
	 * Windows perform about 100 counter per second per thread.
	 */
        void TestSleep()
        {
            std::atomic<unsigned long long> counter = 0;

            std::vector<std::thread> threads;
            for (auto i = 0; i < 10; i++)
            {
                threads.emplace_back([&counter]()
                    {
                        auto start = std::chrono::system_clock::now();
                        while (true)
                        {
                            counter++;
                            std::this_thread::sleep_for(std::chrono::microseconds(1));
                            //std::this_thread::sleep_for(std::chrono::milliseconds(1));

                            auto end = std::chrono::system_clock::now();
                            if ((end - start) > 1000ms)
                            {
                                start = end;
                                std::cout << counter << std::endl;
                                counter = 0;
                            }

                        }
                    });
            }

            threads.front().join();
        }

        void TestPeriodicExecutor()
        {
            PeriodicExecutor pe([]()
                {
#define RUNS_NUMBER 500
                    static std::atomic<long long> counter = 0;
                    if (counter++ % RUNS_NUMBER == 0)
                        std::cout << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl << "counter: " << counter << std::endl;

                    std::this_thread::sleep_for(1ms);
                }, std::chrono::milliseconds(1000 / RUNS_NUMBER));

            while (true)
                pe.Run();
        }
    }
}
