#include <chrono>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <thread>

#include "ConnectionAddr.hpp"
#include "FastTransportProtocol.hpp"
#include "IPacket.hpp"
#include "IStatistics.hpp"
#include "Test.hpp"
#include "UDPQueue.hpp"

using namespace FastTransport::Protocol; // NOLINT

void Test()
{
    TestConnection();
    // TestCloseConnection();
    // TestBBQState();
    // TestTimeRangedStats();
    // TestMultiList();
    // TestSocket();
    // TestRecvQueue();
    // TestSleep();
    // TestPeriodicExecutor();

    /* std::this_thread::sleep_for(std::chrono::milliseconds(100000)); */
}

constexpr int Source = 1;
constexpr int Destination = 2;

void RunSourceConnection(std::string_view srcAddress, uint16_t srcPort, std::string_view dstAddress, uint16_t dstPort)
{
    std::jthread sendThread([srcAddress, srcPort, dstAddress, dstPort](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr(srcAddress, srcPort));
        const ConnectionAddr dstAddr(dstAddress, dstPort);

        const IConnection::Ptr srcConnection = src.Connect(dstAddr);

        IPacket::List userData = UDPQueue::CreateBuffers(200000);
        size_t packetsPerSecond = 0;
        auto start = std::chrono::steady_clock::now();
        const IStatistics& statistics = srcConnection->GetStatistics();
        while (!stop.stop_requested()) {

            if (!userData.empty()) {
                packetsPerSecond += userData.size();
            }
            userData = srcConnection->Send(stop, std::move(userData));

            auto duration = std::chrono::steady_clock::now() - start;
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

int main(int argc, char** argv)
{
#ifdef WIN32
    WSADATA wsaData;
    int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error) {
        throw std::runtime_error("Socket: failed toWSAStartup");
    }
#endif
    if (argc == 1) {
        Test();
        return 0;
    }
    auto args = std::span(argv, argc);

    const int version = std::stoi(args[1]);
    const std::string_view srcAddress = args[2];
    const uint16_t srcPort = std::stoi(args[3]);
    const std::string_view dstAddress = args[4];
    const uint16_t dstPort = std::stoi(args[5]);

    switch (version) {
    case Source: {
        RunSourceConnection(srcAddress, srcPort, dstAddress, dstPort);
        break;
    }
    case Destination: {
        RunDestinationConnection(srcAddress, srcPort);
        break;
    }
    default:
        return 1;
    }

    const int port = std::stoi(args[2]);
    const ConnectionAddr address("0.0.0.0", port);
    const int threadCount = std::stoi(args[3]);
    const int sendBufferSize = std::stoi(args[4]);
    const int recvBufferSize = std::stoi(args[5]);

    const std::string dstIP = args[7];
    UDPQueue socket(address, threadCount, sendBufferSize, recvBufferSize);

    socket.Init();

    const IPacket::List recvBuffers = UDPQueue::CreateBuffers(10000);
    const IPacket::List sendBuffers = UDPQueue::CreateBuffers(10000);

    return 0;
}
