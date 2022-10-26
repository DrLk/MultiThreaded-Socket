#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include "FastTransportProtocol.hpp"
#include "IPacket.hpp"
#include "MultiList.hpp"
#include "PeriodicExecutor.hpp"
#include "RangedList.hpp"
#include "SpeedController.hpp"

namespace FastTransport::Protocol {
void TestConnection()
{
    ConnectionAddr addr("127.0.0.1", 8);
    FastTransportContext src(10100);
    FastTransportContext dst(10200);
    ConnectionAddr srcAddr("127.0.0.1", 10100);
    ConnectionAddr dstAddr("127.0.0.1", 10200);

    IConnection* srcConnection = src.Connect(dstAddr);
    for (int i = 0; i < 20000; i++) {
        IPacket::Ptr data = std::make_unique<Packet>(1500);
        srcConnection->Send(std::move(data));
    }

    IConnection* dstConnection = nullptr;
    while (dstConnection == nullptr) {
        dstConnection = dst.Accept();

        if (dstConnection != nullptr) {
            break;
        }

        std::this_thread::sleep_for(500ms);
    }

    IPacket::List recvPackets;

    for (int i = 0; i < 20000; i++) {
        IPacket::Ptr recvPacket = std::make_unique<Packet>(1500);
        recvPackets.push_back(std::move(recvPacket));
    }

    auto start = std::chrono::steady_clock::now();
    while (true) {
        static size_t totalCount = 0;
        static size_t countPerSecond;
        recvPackets = dstConnection->Recv(std::move(recvPackets));
        if (!recvPackets.empty()) {
            totalCount += recvPackets.size();
            countPerSecond += recvPackets.size();
            std::cout << "Recv packets : " << totalCount << std::endl;
        }

        auto duration = std::chrono::steady_clock::now() - start;
        if (duration > 1s) {
            std::cout << "Recv speed: " << countPerSecond << "pkt/sec" << std::endl;
            countPerSecond = 0;
            start = std::chrono::steady_clock::now();
        }

        // std::this_thread::sleep_for(500ms);
    }
}

void TestTimer()
{
    unsigned long long counter = 0;
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    // Some computation here
    while (true) {
        counter++;
        auto end = clock::now();

        auto elapsed_seconds = end - start;

        if (elapsed_seconds > 5000ms) {
            break;
        }
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

    std::vector<std::thread> threads(10);
    for (auto i = 0; i < 10; i++) {
        threads.emplace_back([&counter]() {
            auto start = std::chrono::system_clock::now();
            while (true) {
                counter++;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                // std::this_thread::sleep_for(std::chrono::milliseconds(1));

                auto end = std::chrono::system_clock::now();
                if ((end - start) > 1000ms) {
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
    constexpr int RUNS_NUMBER = 500;
    PeriodicExecutor pe([]() {
        static std::atomic<long long> counter = 0;
        if (counter++ % RUNS_NUMBER == 0) {
            std::cout << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl
                      << "counter: " << counter << std::endl;
        }

        std::this_thread::sleep_for(1ms);
    },
        std::chrono::milliseconds(1000 / RUNS_NUMBER));

    while (true) {
        pe.Run();
    }
}

void TestRecvQueue()
{
    RecvQueue queue;

    std::vector<IPacket::Ptr> packets;
    constexpr SeqNumberType beginSeqNumber = 0;
    for (int i = 0; i < 10; i++) {
        IPacket::Ptr packet(new Packet(1500));
        packet->GetHeader().SetSeqNumber(i + beginSeqNumber);
        packets.push_back(std::move(packet));
    }

    IPacket::Ptr packet = std::make_unique<Packet>(1500);
    for (auto& packet : packets) {
        queue.AddPacket(std::move(packet));
    }

    queue.ProccessUnorderedPackets();

    auto data = queue.GetUserData();
}

void TestSocket()
{
    Socket src(10100);
    Socket dst(10200);

    src.Init();
    dst.Init();

    std::vector<unsigned char> data(1500);
    ConnectionAddr dstAddr("127.0.0.1", 10200);
    auto result = src.SendTo(data, dstAddr.GetAddr());

    sockaddr_storage addr {};
    std::vector<unsigned char> data2(1500);
    dst.WaitRead();
    auto result2 = dst.RecvFrom(data2, addr);
    std::cout << result << " " << result2;
    int i = 0;
    i++;
}

void TestMultiList()
{
    FastTransport::Containers::MultiList<long long> list;

    for (int i = 0; i < 1000; i++) {
        int number = i;
        list.push_back(std::move(number));
    }

    for (const auto& e : list) {
        std::cout << e << std::endl;
    }

    for (auto& e : list) {
        std::cout << e << std::endl;
    }
}

void TestRangedList()
{
    RangedList list;
    auto now = SampleStats::clock::now();
    list.AddPacket(false, now);
    list.AddPacket(false, now + std::chrono::seconds(15));
}

} // namespace FastTransport::Protocol
