#include <cstdio>

#include "FastTransportProtocol.hpp"
#include "IConnectionState.hpp"
#include "Test.hpp"
#include "UDPQueue.hpp"

#include <algorithm>
#include <list>
#include <memory>
#include <thread>

using namespace FastTransport::Protocol; // NOLINT

void Test()
{
    TestConnection();
    // TestBBQState();
    // TestTimeRangedStats();
    // TestMultiList();
    // TestSocket();
    // TestRecvQueue();
    // TestTimer();
    // TestSleep();
    // TestPeriodicExecutor();

    sockaddr_in dstAddr {};
    // zero out the structure
    std::memset(&dstAddr, 0, sizeof(dstAddr));

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(9999);
    auto result = inet_pton(AF_INET, "127.0.0.1", &(dstAddr.sin_addr));
    // auto result = inet_pton(AF_INET, "8.8.8.8", &(dstAddr.sin_addr));
    // auto result = inet_pton(AF_INET, "192.168.1.56", &(dstAddr.sin_addr));

    if (result == 0) {
        std::cout << errno;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}

int main(int argc, char** argv)
{
#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    if (argc == 1) {
        Test();
    }

    const std::span<char*> args = { argv, static_cast<size_t>(argc) };
    const int port = std::stoi(args[2]);
    const int threadCount = std::stoi(args[3]);
    const int sendBufferSize = std::stoi(args[4]);
    const int recvBufferSize = std::stoi(args[5]);

    const unsigned short dstPort = static_cast<unsigned short>(std::stoi(args[6]));
    const std::string dstIP = args[7];
    UDPQueue socket(port, threadCount, sendBufferSize, recvBufferSize);

    socket.Init();

    const IPacket::List recvBuffers = UDPQueue::CreateBuffers(10000);
    const IPacket::List sendBuffers = UDPQueue::CreateBuffers(10000);

    sockaddr_in dstAddr {};
    // zero out the structure
    std::memset(&dstAddr, 0, sizeof(dstAddr));

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(dstPort);
    auto result = inet_pton(AF_INET, dstIP.c_str(), &(dstAddr.sin_addr));
    if (result != 0) {
        std::cout << errno;
    }
    // auto result = inet_pton(AF_INET, "127.0.0.1", &(dstAddr.sin_addr));
    // auto result = inet_pton(AF_INET, "8.8.8.8", &(dstAddr.sin_addr));
    // auto result = inet_pton(AF_INET, "192.168.1.56", &(dstAddr.sin_addr));

    return 0;
}
