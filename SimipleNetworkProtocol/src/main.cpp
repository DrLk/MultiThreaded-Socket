#include <cstdio>

#include "Test.h"
#include "LinuxUDPQueue.h"
#include "FastTransportProtocol.h"
#include "BufferOwner.h"
#include "IConnectionState.h"

#include <memory>
#include <algorithm>
#include <thread>
#include <list>


using namespace FastTransport::UDPQueue;
using namespace FastTransport::Protocol;

void Test()
{
    TestConnection();

    LinuxUDPQueue srcSocket(8888, 5, 1000, 1000);
    LinuxUDPQueue dstSocket(9999, 5, 1000, 1000);

    srcSocket.Init();
    dstSocket.Init();


    std::list<Packet*> recvBuffers = dstSocket.CreateBuffers(10000);
    std::list<Packet*> sendBuffers = srcSocket.CreateBuffers(10000);

    sockaddr_in dstAddr;
    // zero out the structure
    memset((char*)& dstAddr, 0, sizeof(dstAddr));

    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(9999);
    auto result = inet_pton(AF_INET, "127.0.0.1", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "8.8.8.8", &(dstAddr.sin_addr));
    //auto result = inet_pton(AF_INET, "192.168.1.56", &(dstAddr.sin_addr));

    if (result)
    { 
        errno;
    }

    std::thread srcThread([&sendBuffers, &dstAddr, &srcSocket]()
        {
            long long counter = 0;
            while (true)
            {
                for (auto& buffer : sendBuffers)
                {
                    buffer->GetAddr() = dstAddr;
                    buffer->GetData().resize(sizeof(counter));
                    long long* data = reinterpret_cast<long long*>(buffer->GetData().data());
                    *data = counter++;
                }

                sendBuffers = srcSocket.Send(std::move(sendBuffers));
                if (sendBuffers.empty())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else
                {
                    int a = 0;
                    a++;
                }
            }
        }
    );

    for (auto& buffer : recvBuffers)
    {
        buffer->GetData().resize(1500);
    }
    std::thread dstThread([&recvBuffers, &dstSocket]()
        {
            auto buffer = std::move(recvBuffers);
            long long maxValue = 0;
            long long packetCount = 0;
            while (true)
            {
                buffer = dstSocket.Recv(std::move(buffer));
                for (auto buffer : buffer)
                {
                    buffer->GetData().resize(1500);
                    long long* data = reinterpret_cast<long long*>(buffer->GetData().data());
                    maxValue = std::max<long long>(*data, maxValue);
                    packetCount++;
                }

                if (buffer.empty())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else
                {
                    int a = 0;
                    a++;
                }
            }
        }
    );


    srcThread.join();
    dstThread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}


int main(int argc, char ** argv)
{
#ifdef POSIX
#else
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
    LinuxUDPQueue socket(port, threadCount, sendBufferSize, recvBufferSize);

    socket.Init();

    std::list<Packet*> recvBuffers = socket.CreateBuffers(10000);
    std::list<Packet*> sendBuffers = socket.CreateBuffers(10000);

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

    std::thread srcThread([&sendBuffers, &dstAddr, &socket, &isSource]()
        {
            while (!isSource)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            long long counter = 0;
            while (true)
            {
                for (auto& buffer : sendBuffers)
                {
                    buffer->GetAddr() = dstAddr;
                    buffer->GetData().resize(sizeof(counter));
                    long long* data = reinterpret_cast<long long*>(buffer->GetData().data());
                    *data = counter++;
                }

                sendBuffers = socket.Send(std::move(sendBuffers));
                if (sendBuffers.empty())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else
                {
                    int a = 0;
                    a++;
                }
            }
        });

    std::thread dstThread([&recvBuffers, &socket]()
        {
            long long maxValue = 0;
            long long packetCount = 0;
            while (true)
            {
                for (auto buffer : recvBuffers)
                {
                    buffer->GetData().resize(1500);
                }

                recvBuffers = socket.Recv(std::move(recvBuffers));
                for (auto buffer : recvBuffers)
                {
                    long long* data = reinterpret_cast<long long*>(buffer->GetData().data());
                    maxValue = std::max<long long>(*data, maxValue);
                    packetCount++;


                    if (packetCount % 100000 == 0)
                    {
                        printf("got new 100k: %lld\n", packetCount);
                    }
                }

                if (recvBuffers.empty())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else
                {
                    int a = 0;
                    a++;
                }
            }
        }
    );


    dstThread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
    return 0;
}


