#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ratio>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#include "Connection.hpp"
#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "ISpeedControllerState.hpp"
#include "LockedList.hpp"
#include "Logger.hpp"
#include "Packet.hpp"
#include "PeriodicExecutor.hpp"
#include "RecvQueue.hpp"
#include "SampleStats.hpp"
#include "Socket.hpp"
#include "TimeRangedStats.hpp"
#include "UDPQueue.hpp"

namespace FastTransport::Protocol {
void TestConnection()
{
    std::jthread recvThread([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));
        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        auto start = std::chrono::steady_clock::now();
        static size_t totalCount = 0;
        while (!stop.stop_requested()) {
            static size_t countPerSecond;
            recvPackets = dstConnection->Recv(stop, std::move(recvPackets));
            if (!recvPackets.empty()) {
                totalCount += recvPackets.size();
                countPerSecond += recvPackets.size();
            }

            auto duration = std::chrono::steady_clock::now() - start;
            if (duration > 1s) {
                std::cout << "Recv packets : " << totalCount << std::endl;
                std::cout << "Recv speed: " << countPerSecond << "pkt/sec" << std::endl;
                countPerSecond = 0;
                start = std::chrono::steady_clock::now();
            }

            // std::this_thread::sleep_for(500ms);
        }
    });

    std::this_thread::sleep_for(500ms);

    std::jthread sendThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List userData = UDPQueue::CreateBuffers(200000);
        while (!stop.stop_requested()) {
            userData = srcConnection->Send(stop, std::move(userData));
        }
    });

    recvThread.join();
    sendThread.join();
}

void Recv(std::stop_token stop, int connectionCount)
{
    FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));
    for (int i = 0; i < connectionCount; i++) {
        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        LOGGER() << "Get " << i << " connections";
        IPacket::List recvPackets = UDPQueue::CreateBuffers(30);
        while (!dstConnection->IsClosed()) {
            recvPackets = dstConnection->Recv(stop, std::move(recvPackets));

            if (!recvPackets.empty()) {
                std::this_thread::sleep_for(500ms);
                break;
            }
        }

        dstConnection->Close();
    }
}

void Send(std::stop_token stop, int connectionCount)
{
    FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
    const ConnectionAddr dstAddr("127.0.0.1", 11200);
    std::vector<IConnection::Ptr> connections;
    for (int i = 0; i < connectionCount; i++) {
        const IConnection::Ptr srcConnection = src.Connect(dstAddr);
        IPacket::List userData = UDPQueue::CreateBuffers(1);
        userData = srcConnection->Send(stop, std::move(userData));
        connections.push_back(srcConnection);
    }

    while (!connections.empty()) {
        std::erase_if(connections, [](const IConnection::Ptr& connection) {
            if (connection->IsClosed()) {
                LOGGER() << "Closed";
                return true;
            }

            return false;
        });

        std::this_thread::sleep_for(50ms);
    }
}

void TestCloseConnection()
{
    const int connectionCount = 5;

    std::jthread recvThread(Recv, connectionCount);

    std::this_thread::sleep_for(500ms);

    std::jthread sendThread(Send, connectionCount);

    recvThread.join();
    sendThread.join();
}

/*
 * Linux rpi4 aarch64 manjaro 13.12.2021 counter about 12k per second per thread.
 * 10 threads perform over 110k counter per seconds
 *
 * Windows perform about 100 counter per second per thread.
 */
void TestSleep()
{
    std::atomic<uint64_t> counter = 0;

    std::vector<std::thread> threads(10);
    for (auto i = 0; i < 1; i++) {
        threads[i] = std::thread([&counter]() {
            auto start = std::chrono::system_clock::now();
            while (true) {
                counter++;
                std::this_thread::sleep_for(10ms);

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
    PeriodicExecutor executor([]() {
        static std::atomic<int64_t> counter = 0;
        if (counter++ % RUNS_NUMBER == 0) {
            std::cout << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl
                      << "counter: " << counter << std::endl;
        }

        // std::this_thread::sleep_for(1ms);
    },
        std::chrono::milliseconds(1000 / RUNS_NUMBER));

    while (true) {
        executor.Run();
    }
}

void TestRecvQueue()
{
    std::unique_ptr<IRecvQueue> queue = std::make_unique<RecvQueue>();

    std::vector<IPacket::Ptr> packets;
    constexpr SeqNumberType beginSeqNumber = 0;
    for (int i = 0; i < 10; i++) {
        IPacket::Ptr packet(new Packet(1400));
        packet->SetSeqNumber(i + beginSeqNumber);
        packets.push_back(std::move(packet));
    }

    const IPacket::Ptr packet = std::make_unique<Packet>(1400);

    IPacket::List freePackets;
    for (auto& packet : packets) {
        auto freePacket = queue->AddPacket(std::move(packet));
        if (freePacket) {
            freePackets.push_back(std::move(freePacket));
        }
    }

    queue->ProccessUnorderedPackets();

    auto& userData = queue->GetUserData();
    IPacket::List result;
    if (userData.Wait(std::stop_token())) {
        userData.LockedSwap(result);
    }
}

void TestSocket()
{
    Socket src(ConnectionAddr("0.0.0.0", 10100));
    Socket dst(ConnectionAddr("0.0.0.0", 10200));

    src.Init();
    dst.Init();

    std::vector<unsigned char> data(1400);
    const ConnectionAddr dstAddr("192.0.0.1", 10200);
    auto result = src.SendTo(data, dstAddr);

    ConnectionAddr addr;
    std::vector<unsigned char> data2(1400);
    while (!dst.WaitRead()) { }
    auto result2 = dst.RecvFrom(data2, addr);
    std::cout << result << " " << result2;
}

void TestMultiList()
{
    FastTransport::Containers::MultiList<int64_t> list;

    for (int i = 0; i < 1000; i++) {
        int number = i;
        list.push_back(std::move(number));
    }

    for (const auto& element : list) {
        std::cout << element << std::endl;
    }

    for (auto& element : list) {
        std::cout << element << std::endl;
    }
}

void TestTimeRangedStats()
{
    TimeRangedStats list;
    auto now = SampleStats::clock::now();
    const SampleStats::clock::duration rtt = 10ms;
    list.AddPacket(false, now, rtt);
    list.AddPacket(false, now + std::chrono::seconds(15), rtt);
    list.AddPacket(false, now + std::chrono::seconds(35), rtt);
}

void TestBBQState()
{
    BBQState state;

    SpeedControllerState speedState {};
    TimeRangedStats stats;
    auto startInterval = std::chrono::steady_clock::now();

    state.Run(stats, speedState);

    for (int i = 0; i < 100; i++) {
        const SampleStats::clock::duration rtt = 100ms;
        stats.AddPacket(false , startInterval, rtt);
        startInterval += 50ms;
    }

    state.Run(stats, speedState);
}

} // namespace FastTransport::Protocol
