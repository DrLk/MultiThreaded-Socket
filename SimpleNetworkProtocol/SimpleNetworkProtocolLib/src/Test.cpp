#include "Test.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "IConnection.hpp"
#include "IPacket.hpp"
#include "UDPQueue.hpp"

using namespace std::chrono_literals;

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

        const auto& statistics = dstConnection->GetStatistics();
        auto start = std::chrono::steady_clock::now();
        while (!stop.stop_requested()) {
            static size_t countPerSecond;
            recvPackets = dstConnection->Recv(stop, std::move(recvPackets));
            if (!recvPackets.empty()) {
                countPerSecond += recvPackets.size();
            }

            auto duration = std::chrono::steady_clock::now() - start;
            if (duration > 1s) {
                std::cout << "Recv speed: " << countPerSecond << "pkt/sec" << '\n';
                std::cout << "dst: " << statistics << '\n';
                countPerSecond = 0;
                start = std::chrono::steady_clock::now();
            }
        }
    });

    std::this_thread::sleep_for(500ms);

    std::jthread sendThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List userData = UDPQueue::CreateBuffers(200000);
        const auto& statistics = srcConnection->GetStatistics();
        auto start = std::chrono::steady_clock::now();
        while (!stop.stop_requested()) {
            userData = srcConnection->Send2(stop, std::move(userData));
            auto duration = std::chrono::steady_clock::now() - start;
            if (duration > 1s) {
                std::cout << "src: " << statistics << '\n';
                start = std::chrono::steady_clock::now();
            }
        }
    });

    recvThread.join();
    sendThread.join();
}

} // namespace FastTransport::Protocol
