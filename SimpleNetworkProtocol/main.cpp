#include <chrono>
#include <cstring>
#include <span>
#include <string>
#include <thread>

#include "ConnectionAddr.hpp"
#include "IPacket.hpp"
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

    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
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
        while (!stop.stop_requested()) {
            userData = srcConnection->Send(stop, std::move(userData));
        } });

    sendThread.join();
}

void RunDestinationConnection(std::string_view srcAddress, uint16_t srcPort, std::string_view dstAddress, uint16_t dstPort)
{
    std::jthread recvThread([srcAddress, srcPort, dstAddress, dstPort](std::stop_token stop) {
        FastTransportContext src(ConnectionAddr(srcAddress, srcPort));
        IPacket::List recvPackets = UDPQueue::CreateBuffers(260000);

        const IConnection::Ptr dstConnection = src.Accept(stop);
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
    }

    int version = std::stoi(argv[1]);

    std::string_view srcAddress = argv[2];
    uint16_t srcPort = std::stoi(argv[3]);
    std::string_view dstAddress = argv[4];
    uint16_t dstPort = std::stoi(argv[5]);

    switch (version) {
    case Source: {
        RunSourceConnection(srcAddress, srcPort, dstAddress, dstPort);
    }
    case Destination: {
        RunDestinationConnection(srcAddress, srcPort, dstAddress, dstPort);
    }
    }

    const std::span<char*> args { argv, static_cast<size_t>(argc) };
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
