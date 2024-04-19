#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "FileSystem/RemoteFileSystem.hpp"
#include "IPacket.hpp"
#include "IStatistics.hpp"
#include "MergeRequest.hpp"
#include "MessageTypeReadJob.hpp"
#include "TaskScheduler.hpp"
#include "Test.hpp"
#include "UDPQueue.hpp"

#include "NativeFile.hpp"

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
    using FastTransport::TaskQueue::TaskScheduler;
    using FastTransport::TaskQueue::MessageTypeReadJob;
    using FastTransport::TaskQueue::MergeRequest;

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

        FileTree fileTree = FileTree("/tmp/test2");
        TaskScheduler destinationTaskScheduler(*dstConnection, fileTree);

        destinationTaskScheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        destinationTaskScheduler.Schedule(MergeRequest::Create());

        using FastTransport::TaskQueue::RemoteFileSystem;
        RemoteFileSystem filesystem("/mnt/test");
        RemoteFileSystem::scheduler = &destinationTaskScheduler;
        filesystem.Start();

        destinationTaskScheduler.Wait(stop);
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

        FileTree fileTree = FileTree::GetTestFileTree();
        TaskScheduler sourceTaskScheduler(*srcConnection, fileTree);

        sourceTaskScheduler.Schedule(MessageTypeReadJob::Create(fileTree, IPacket::List()));

        sourceTaskScheduler.Wait(stop);
    });

    recvThread.join();
    sendThread.join();
}

void TestConnection3()
{
    using NativeFile = FastTransport::FileSystem::NativeFile;
    using FastTransport::TaskQueue::RemoteFileSystem;

    RemoteFileSystem filesystem("/mnt/test");
    filesystem.Start();
    const NativeFile file("/mnt/test/test.txt", 10, std::filesystem::file_type::regular);
}

void TestReadV()
{
    auto file = open("/tmp/test1", O_RDONLY | O_CLOEXEC); // NOLINT(hicpp-vararg, cppcoreguidelines-pro-type-vararg)

    const int blocks = 2;
    std::vector<iovec> iovecs(blocks);
    std::array<unsigned char, 1400> buffer1 {};
    std::array<unsigned char, 1400> buffer2 {};
    iovecs[0].iov_base = buffer1.data();
    iovecs[0].iov_len = buffer1.size();
    iovecs[1].iov_base = buffer2.data();
    iovecs[1].iov_len = buffer2.size();

    const std::int64_t result = preadv(file, iovecs.data(), blocks, 11);
    assert(result != -1);
}

int main(int argc, char** argv)
{
    TestConnection2();
    return 0;
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
