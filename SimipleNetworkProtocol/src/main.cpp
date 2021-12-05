#include <cstdio>

#include "UDPQueue.h"
#include "FastTransportProtocol.h"
#include "IConnectionState.h"
#include "Test.h"

#include <memory>
#include <algorithm>
#include <thread>
#include <list>


using namespace FastTransport::Protocol;

void Test()
{
    //TestConnection();
    TestRecvQueue();
    //TestTimer();
    //TestSleep();
    //TestPeriodicExecutor();

    sockaddr_in dstAddr;
    // zero out the structure
    memset((char*)& dstAddr, 0, sizeof(dstAddr));

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(9999);
    auto result = inet_pton(AF_INET, "127.0.0.1", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "8.8.8.8", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "192.168.1.56", &(dstAddr.sin_addr));

    if (!result)
    { 
        errno;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}


int main(int argc, char ** argv)
{
#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    if (argc == 1)
        Test();

    bool isSource = false;
    if (std::string(argv[1]) == "src")
    {
        isSource = true;
    }


    int port = std::stoi(argv[2]);
    int threadCount = std::stoi(argv[3]);
    int sendBufferSize = std::stoi(argv[4]);
    int recvBufferSize = std::stoi(argv[5]);

    unsigned short dstPort = (unsigned short)std::stoi(argv[6]);
    std::string dstIP = argv[7];
    UDPQueue socket(port, threadCount, sendBufferSize, recvBufferSize);

    socket.Init();

    std::list<std::unique_ptr<IPacket>> recvBuffers = socket.CreateBuffers(10000);
    std::list<std::unique_ptr<IPacket>> sendBuffers = socket.CreateBuffers(10000);

    sockaddr_in dstAddr;
    // zero out the structure
    memset((char*)& dstAddr, 0, sizeof(dstAddr));

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(dstPort);
    auto result = inet_pton(AF_INET, dstIP.c_str(), &(dstAddr.sin_addr));
    if (result)
        errno;
    //auto result = inet_pton(AF_INET, "127.0.0.1", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "8.8.8.8", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "192.168.1.56", &(dstAddr.sin_addr));

    return 0;
}


