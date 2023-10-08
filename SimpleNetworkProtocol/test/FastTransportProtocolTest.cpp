#include "FastTransportProtocol.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <exception>
#include <thread>

namespace FastTransport::Protocol {

using namespace std::chrono_literals;

TEST(FastTransportProtocolTest, ConnectDestinationFirst)
{
    static constexpr auto TestTimeout = 10s;

    std::jthread recvThread([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));
        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        auto start = std::chrono::steady_clock::now();
        while (!stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - recvThread";
                return;
            }

            std::this_thread::sleep_for(100ms);
        }
    });

    std::this_thread::sleep_for(500ms);

    std::jthread sendThread([&recvThread](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        auto start = std::chrono::steady_clock::now();
        while (!srcConnection->IsConnected() && !stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - sendThread";
                return;
            }

            std::this_thread::sleep_for(100ms);
        }

        recvThread.request_stop();
    });

    sendThread.join();
    recvThread.join();
}

TEST(FastTransportProtocolTest, ConnectSourceFirst)
{
    static constexpr auto TestTimeout = 10s;

    std::jthread sendThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        auto start = std::chrono::steady_clock::now();
        while (!srcConnection->IsConnected() && !stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - sendThread";
                return;
            }

            std::this_thread::sleep_for(100ms);
        }
    });

    std::this_thread::sleep_for(500ms);

    std::jthread recvThread([&sendThread](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));
        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        auto start = std::chrono::steady_clock::now();
        while (!stop.stop_requested() && sendThread.joinable()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - recvThread";
                return;
            }

            std::this_thread::sleep_for(100ms);
        }
    });

    sendThread.join();
    recvThread.join();
}

}