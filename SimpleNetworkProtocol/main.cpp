#include <chrono>
#include <cstring>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <thread>

#include "ConnectionAddr.hpp"
#include "ConnectionReader.hpp"
#include "ConnectionWriter.hpp"
#include "FastTransportProtocol.hpp"
#include "IPacket.hpp"
#include "IStatistics.hpp"
#include "MergeIn.hpp"
#include "MergeOut.hpp"
#include "MergeRequest.hpp"
#include "MessageReader.hpp"
#include "MessageTypeReadJob.hpp"
#include "MessageWriter.hpp"
#include "NetworkStream.hpp"
#include "TaskScheduler.hpp"
#include "Test.hpp"
#include "UDPQueue.hpp"

#include "Logger.hpp"

using FastTransport::FileSystem::FileTree;

using namespace FastTransport::Protocol; // NOLINT

constexpr int Source = 1;
constexpr int Destination = 2;

void RunSourceConnection(std::string_view srcAddress, uint16_t srcPort, std::string_view dstAddress, uint16_t dstPort, std::optional<size_t> sendSpeed)
{
    auto endTestTime = std::chrono::steady_clock::now() + 20s;
    std::jthread sendThread([srcAddress, srcPort, dstAddress, dstPort, endTestTime, sendSpeed](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr(srcAddress, srcPort));
        const ConnectionAddr dstAddr(dstAddress, dstPort);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        if (sendSpeed.has_value()) {
            srcConnection->GetContext().SetInt(Settings::MinSpeed, sendSpeed.value());
            srcConnection->GetContext().SetInt(Settings::MaxSpeed, sendSpeed.value());
        }

        IPacket::List userData = UDPQueue::CreateBuffers(200000);
        size_t packetsPerSecond = 0;
        auto start = std::chrono::steady_clock::now();
        const IStatistics& statistics = srcConnection->GetStatistics();
        while (!stop.stop_requested()) {

            if (!userData.empty()) {
                packetsPerSecond += userData.size();
            }
            userData = srcConnection->Send2(stop, std::move(userData));

            auto now = std::chrono::steady_clock::now();
            if (now > endTestTime) { return;
            }

            auto duration = now - start;
            if (duration > 1s) {
                std::cout << statistics << '\n';
                std::cout << "Send speed: " << packetsPerSecond << "pkt/sec" << '\n';
                packetsPerSecond = 0;
                start = std::chrono::steady_clock::now();
            }
        } });

    sendThread.join();
}

void RunDestinationConnection(std::string_view srcAddress, uint16_t srcPort)
{
    std::jthread recvThread([srcAddress, srcPort](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr(srcAddress, srcPort));
        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);

        const IConnection::Ptr dstConnection = src.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        auto start = std::chrono::steady_clock::now();
        size_t packetsPerSecond = 0;
        const IStatistics& statistics = dstConnection->GetStatistics();
        while (!stop.stop_requested()) {

            recvPackets = dstConnection->Recv(stop, std::move(recvPackets));
            if (!recvPackets.empty()) {
                packetsPerSecond += recvPackets.size();
            }

            auto duration = std::chrono::steady_clock::now() - start;
            if (duration > 1s) {
                std::cout << statistics << '\n';
                std::cout << "Recv speed: " << packetsPerSecond << "pkt/sec" << '\n';
                packetsPerSecond = 0;
                start = std::chrono::steady_clock::now();
            }

            // std::this_thread::sleep_for(500ms);
        } });

    recvThread.join();
}

void TestConnection2()
{
    using namespace FastTransport::TaskQueue;

    std::jthread recvThread([](std::stop_token stop) {
        FastTransportContext dst(ConnectionAddr("127.0.0.1", 11200));

        const IConnection::Ptr dstConnection = dst.Accept(stop);
        if (dstConnection == nullptr) {
            throw std::runtime_error("Accept return nullptr");
        }

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        dstConnection->AddFreeRecvPackets(std::move(recvPackets));
        dstConnection->AddFreeSendPackets(std::move(sendPackets));

        TaskScheduler destinationTaskScheduler(*dstConnection);

        FileTree fileTree;
        destinationTaskScheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        destinationTaskScheduler.Schedule(MergeRequest::Create());

        destinationTaskScheduler.Wait(stop);

        const auto& statistics = dstConnection->GetStatistics();
        auto start = std::chrono::steady_clock::now();

        ConnectionWriter writer(stop, dstConnection);
        ConnectionReader reader(stop, dstConnection);
        FastTransport::FileSystem::OutputByteStream<ConnectionWriter> output(writer);
        FastTransport::FileSystem::InputByteStream<ConnectionReader> input(reader);
        FastTransport::Protocol::NetworkStream<ConnectionReader, ConnectionWriter> networkStream(reader, writer);

        IPacket::List freeSendPackets = dstConnection->Send2(stop, IPacket::List());
        IPacket::List messagePackets = freeSendPackets.TryGenerate(10);
        MessageWriter message(std::move(messagePackets));
        FastTransport::FileSystem::OutputByteStream<MessageWriter> output2(message);
        IPacket::List recvPackets2 = dstConnection->Recv(stop, IPacket::List());
        IPacket::List recvMessagePackets = recvPackets2.TryGenerate(10);
        MessageReader messageReader(std::move(recvMessagePackets));
        FastTransport::FileSystem::InputByteStream<MessageReader> input2(messageReader);
        FastTransport::TaskQueue::MergeOut mergeOut(fileTree);
        FastTransport::TaskQueue::MergeIn mergeIn(fileTree, std::move(messageReader));

        int a = 0;
        input >> a;
        LOGGER() << "Read a: " << a;

        int a2 = 0;
        input >> a2;
        LOGGER() << "Read a2: " << a2;

        uint64_t b = 321;
        output << b;
        LOGGER() << "Write b: " << b;
        output.Flush();

        /* Protocol protocol(output, input); */
        /* protocol.Run(); */

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

    std::this_thread::sleep_for(1s);

    std::jthread sendThread([](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr("127.0.0.1", 11100));
        const ConnectionAddr dstAddr("127.0.0.1", 11200);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);
        IPacket::List sendPackets = UDPQueue::CreateBuffers(200000);
        srcConnection->AddFreeRecvPackets(std::move(recvPackets));
        srcConnection->AddFreeSendPackets(std::move(sendPackets));

        TaskScheduler sourceTaskScheduler(*srcConnection);

        FileTree fileTree = FileTree::GetTestFileTree();
        sourceTaskScheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        sourceTaskScheduler.Wait(stop);

        const auto& statistics = srcConnection->GetStatistics();
        auto start = std::chrono::steady_clock::now();

        ConnectionWriter writer(stop, srcConnection);
        ConnectionReader reader(stop, srcConnection);
        FastTransport::FileSystem::OutputByteStream<ConnectionWriter> output(writer);
        FastTransport::FileSystem::InputByteStream<ConnectionReader> input(reader);

        int a = 123;
        output << a;
        LOGGER() << "Write a: " << a;
        output.Flush();

        int a2 = 124;
        output << a2;
        LOGGER() << "Write a2: " << a2;
        output.Flush();

        uint64_t b = 0;
        input >> b;
        LOGGER() << "Read b: " << b;
        std::this_thread::sleep_for(500s);

        /* Protocol protocol(output, input); */
        /* protocol.Run(); */
        /*  */
        /* FastTransport::FileSystem::FileSystem filesystem; */
        /* filesystem.Start(); */

        while (!stop.stop_requested()) {
            sendPackets = srcConnection->Send2(stop, std::move(sendPackets));
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

int main(int argc, char** argv)
{
    TestConnection2();
#ifdef WIN32
    WSADATA wsaData;
    int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error) {
        throw std::runtime_error("Socket: failed toWSAStartup");
    }
#endif
    if (argc == 1) {
        TestConnection();
        return 0;
    }
    auto args = std::span(argv, argc);

    const int version = std::stoi(args[1]);
    const std::string_view srcAddress = args[2];
    const uint16_t srcPort = std::stoi(args[3]);
    const std::string_view dstAddress = args[4];
    const uint16_t dstPort = std::stoi(args[5]);
    std::optional<size_t> sendSpeed;
    if (argc > 6) {
        sendSpeed = std::stoi(args[6]);
    }

    switch (version) {
    case Source: {
        RunSourceConnection(srcAddress, srcPort, dstAddress, dstPort, sendSpeed);
        break;
    }
    case Destination: {
        RunDestinationConnection(srcAddress, srcPort);
        break;
    }
    default:
        return 1;
    }

    return 0;
}
