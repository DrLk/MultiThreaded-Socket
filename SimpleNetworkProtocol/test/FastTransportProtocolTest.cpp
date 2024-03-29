#include "FastTransportProtocol.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <future>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <utility>

#include "Connection.hpp"
#include "IPacket.hpp"
#include "UDPQueue.hpp"

using namespace std::chrono_literals;

namespace FastTransport::Protocol {

TEST(FastTransportProtocolTest, ConnectDestinationFirst) // NOLINT(readability-function-cognitive-complexity)
{
    std::packaged_task<void(std::stop_token stop)> recvTask([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11300));

        int connectionCount = 0;
        static constexpr int ExpectedConnecionCount = 2;
        while (!stop.stop_requested() && connectionCount < ExpectedConnecionCount) {
            const IConnection::Ptr dstConnection = dst.Accept(stop);
            if (dstConnection == nullptr) {
                throw std::runtime_error("Accept return nullptr");
            }

            auto ping = UDPQueue::CreateBuffers(1000);
            ping = dstConnection->Recv(stop, std::move(ping));
            auto pong = UDPQueue::CreateBuffers(1);
            pong = dstConnection->Send2(stop, std::move(pong));

            connectionCount++;
        }

        std::this_thread::sleep_for(1s);
    });

    auto recvReady = recvTask.get_future();
    std::jthread recvThread(std::move(recvTask));

    std::this_thread::sleep_for(500ms);

    std::packaged_task<void(std::stop_token stop)> sendTask1([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11300);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        while (!srcConnection->IsConnected() && !stop.stop_requested()) {
            std::this_thread::sleep_for(100ms);
        }

        auto ping = UDPQueue::CreateBuffers(1);
        ping = srcConnection->Send2(stop, std::move(ping));

        auto pong = UDPQueue::CreateBuffers(1000);
        while (!stop.stop_requested()) {
            pong = srcConnection->Recv(stop, std::move(pong));
            if (!pong.empty()) {
                break;
            }
        }

        std::this_thread::sleep_for(1s);
    });

    auto sendReady1 = sendTask1.get_future();
    std::jthread sendThread1(std::move(sendTask1));

    std::packaged_task<void(std::stop_token stop)> sendTask2([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11200));
        const ConnectionAddr dstAddr("127.0.0.1", 11300);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        while (!srcConnection->IsConnected() && !stop.stop_requested()) {
            std::this_thread::sleep_for(100ms);
        }
        auto ping = UDPQueue::CreateBuffers(1);
        ping = srcConnection->Send2(stop, std::move(ping));

        auto pong = UDPQueue::CreateBuffers(1000);
        while (!stop.stop_requested()) {
            pong = srcConnection->Recv(stop, std::move(pong));
            if (!pong.empty()) {
                break;
            }
        }

        std::this_thread::sleep_for(1s);
    });

    auto sendReady2 = sendTask2.get_future();
    std::jthread sendThread2(std::move(sendTask2));

    static constexpr auto TestTimeout = 10s;
    auto start = std::chrono::steady_clock::now();

    auto running = [&recvReady, &sendReady1, &sendReady2] {
        return recvReady.wait_for(100ms) != std::future_status::ready || sendReady2.wait_for(100ms) != std::future_status::ready || sendReady1.wait_for(100ms) != std::future_status::ready;
    };

    while (running()) {
        if (std::chrono::steady_clock::now() - start > TestTimeout) {
            EXPECT_TRUE(!running()) << "Timeout";
            sendThread1.request_stop();
            sendThread2.request_stop();
            recvThread.request_stop();
            break;
        }

        std::this_thread::sleep_for(100ms);
    }

    sendThread1.join();
    sendThread2.join();
    recvThread.join();
}

TEST(FastTransportProtocolTest, ConnectSourceFirst)
{
    static constexpr auto TestTimeout = 10s;

    std::jthread sendThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 21100));
        const ConnectionAddr dstAddr("127.0.0.1", 21200);

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
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 21200));

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

TEST(FastTransportProtocolTest, PayloadTest)
{
    static constexpr auto TestTimeout = 10s;

    std::array<IPacket::ElementType, 100> data {};

    for (int i = 0; i < 100; i++) {
        data.at(i) = static_cast<std::byte>(i);
    }

    std::packaged_task<void(std::stop_token stop)> pingTask([&data](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 21100));
        const ConnectionAddr dstAddr("127.0.0.1", 21200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        auto start = std::chrono::steady_clock::now();
        while (!srcConnection->IsConnected() && !stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - sendThread";
                return;
            }

            std::this_thread::sleep_for(100ms);
        }

        auto ping = UDPQueue::CreateBuffers(1);
        ping.front()->SetPayload(data);
        ping = srcConnection->Send2(stop, std::move(ping));

        auto pong = UDPQueue::CreateBuffers(1000);
        start = std::chrono::steady_clock::now();
        while (!stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - recvThread";
                return;
            }

            pong = srcConnection->Recv(stop, std::move(pong));
            if (!pong.empty()) {
                auto payload = pong.front()->GetPayload();
                EXPECT_TRUE(std::equal(data.begin(), data.end(), payload.begin(), payload.end()));
                break;
            }

            std::this_thread::sleep_for(100ms);
        }
    });

    auto ready = pingTask.get_future();
    const std::jthread pingThread(std::move(pingTask));

    std::jthread pongThread([&ready, &data](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 21200));

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        auto ping = UDPQueue::CreateBuffers(1);
        auto start = std::chrono::steady_clock::now();
        while (!stop.stop_requested()) {
            if (std::chrono::steady_clock::now() - start > TestTimeout) {
                EXPECT_TRUE(false) << "Timeout waiting - recvThread";
                return;
            }

            ping = dstConnection->Recv(stop, std::move(ping));
            if (!ping.empty()) {
                const auto& payload = ping.front()->GetPayload();
                EXPECT_TRUE(std::equal(data.begin(), data.end(), payload.begin(), payload.end()));

                auto pong = UDPQueue::CreateBuffers(1);
                ping.front()->SetPayload(payload);
                ping = dstConnection->Send2(stop, std::move(ping));

                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        auto status = ready.wait_for(TestTimeout);

        EXPECT_TRUE(status == std::future_status::ready);
    });

    pongThread.join();
}

} // namespace FastTransport::Protocol
