#include <chrono>
#include <cstdint>
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
    // TestLogger();
    // TestBBQState();
    // TestTimeRangedStats();
    // TestMultiList();
    // TestSocket();
    // TestRecvQueue();
    // TestTimer();
    // TestSleep();
    // TestPeriodicExecutor();

    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
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
