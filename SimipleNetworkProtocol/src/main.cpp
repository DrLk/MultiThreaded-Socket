#include <cstdio>

#include "LinuxUDPQueue.h"
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
    //TestTimer();
    //TestConnection();
    TestSleep();
    //TestPeriodicExecutor();

    LinuxUDPQueue srcSocket(8888, 50, 1000, 100);
    //LinuxUDPQueue dstSocket(9999, 50, 1000, 100);

    srcSocket.Init();
    //dstSocket.Init();


    std::list<std::unique_ptr<IPacket>> sendBuffers = srcSocket.CreateBuffers(100000);
    //std::list<std::unique_ptr<IPacket>> recvBuffers = dstSocket.CreateBuffers(100000);

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

    srcSocket.OnSend = [](const std::list<std::unique_ptr<IPacket>>& packets) {
        static std::atomic<unsigned long long> _counter = 0;
        static auto start = std::chrono::system_clock::now();
        auto end = std::chrono::system_clock::now();
        auto duration = start - end;
        auto elapsed_seconds = end - start;

        _counter += packets.size();
        if (elapsed_seconds > 1000ms)
        {
            std::cout << _counter << std::endl;
            start = end;
            _counter = 0;
        }

        std::this_thread::sleep_for(1ms);

    };

    std::thread srcThread([&sendBuffers, &dstAddr, &srcSocket]()
        {
            long long counter = 0;
            while (true)
            {
                for (auto& buffer : sendBuffers)
                {
                    /*
                    buffer->GetAddr() = dstAddr;
                    buffer->GetData().resize(sizeof(counter));
                    const long long* data = reinterpret_cast<const long long*>(buffer->GetPayload().data());
                    *data = counter++;
                    */
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

    /*std::thread dstThread([&recvBuffers, &dstSocket]()
        {
            auto buffer = std::move(recvBuffers);
            long long maxValue = 0;
            long long packetCount = 0;
            while (true)
            {
                buffer = dstSocket.Recv(std::move(buffer));
                for (auto& buffer : buffer)
                {
                    //buffer->GetPayload().resize(1500);
                    //const long long* data = reinterpret_cast<const long long*>(buffer->GetPayload().data());
                    //maxValue = std::max<long long>(*data, maxValue);
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
    );*/


    srcThread.join();
    //dstThread.join();
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
    LinuxUDPQueue socket(port, threadCount, sendBufferSize, recvBufferSize);

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

    std::thread srcThread([&sendBuffers, &dstAddr, &socket, &isSource]()
        {
            while (!isSource)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));

            long long counter = 0;
            while (true)
            {
                for (auto& buffer : sendBuffers)
                {
                    /*buffer->GetAddr() = dstAddr;
                    buffer->GetData().resize(sizeof(counter));
                    const long long* data = reinterpret_cast<const long long*>(buffer->GetPayload().data());
                    *data = counter++;*/
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
                /*for (auto buffer : recvBuffers)
                {
                    buffer->GetPayload().resize(1500);
                }*/

                recvBuffers = socket.Recv(std::move(recvBuffers));
                for (const auto& buffer : recvBuffers)
                {
                    //const long long* data = reinterpret_cast<const long long*>(buffer->GetPayload().data());
                    //maxValue = std::max<long long>(*data, maxValue);
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


