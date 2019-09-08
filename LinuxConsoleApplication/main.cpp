#include <cstdio>

#include "LinuxUDPQueue.h"




int main(void)
{
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
                    usleep(1);
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
            long long maxValue = 0;
            long long packetCount = 0;
            while (true)
            {
                recvBuffers = dstSocket.Recv(std::move(recvBuffers));
                for (auto& buffer : recvBuffers)
                {
                    buffer->GetData().resize(1500);
                    long long *data = reinterpret_cast<long long*>(buffer->GetData().data());
                    maxValue = std::max(*data, maxValue);
                    packetCount++;
                }

                if (recvBuffers.empty())
                    usleep(1);
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
    sleep(10000);
    return 0;
}