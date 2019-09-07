// MS Network API test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <memory>
#include "SelectPipe.h"
#include <ws2tcpip.h>

int main()
{
    std::unique_ptr<IPipe> pipe = std::make_unique<SelectPipe>();
    pipe->Listen(8889);

    std::list<IPacket*> readPackets = pipe->CreatePackets(1300, 1);

    readPackets = pipe->Read(readPackets);
    std::list<IPacket*> writePackets = pipe->CreatePackets(1300, 10000); //TODO: remove

    std::vector<char> bytes;
    for (int i = 0; i < 1; i++)
    {
        bytes.push_back('Q');
    }

    SOCKADDR_IN dst;
    dst.sin_family = AF_INET;
    //inet_pton(AF_INET, "127.0.0.1", &(dst.sin_addr));
    //inet_pton(AF_INET, "192.168.1.1", &(dst.sin_addr));
    //inet_pton(AF_INET, "10.10.1.1", &(dst.sin_addr));
    inet_pton(AF_INET, "8.8.8.8", &(dst.sin_addr));
    dst.sin_port = htons(7789);

    for (auto packet : writePackets)
    {
        packet->SetBuffer(bytes);
        packet->GetDestination() = dst;
    }

    /*std::unique_ptr<IPipe> pipe2 = std::make_unique<SelectPipe>();
    pipe2->Listen(8888);

    std::list<IPacket*> writePackets2 = pipe2->CreatePackets(1300, 10000); //TODO: remove
    inet_pton(AF_INET, "192.168.1.1", &(dst.sin_addr));
    dst.sin_port = htons(7788);

    for (auto packet : writePackets2)
    {
        packet->SetBuffer(bytes);
        packet->GetDestination() = dst;
    }
    */
    //std::unique_ptr<IPipe> pipe3 = std::make_unique<SelectPipe>();
    //pipe3->Listen();
    while (true)
    {
        Sleep(1000);
    //    writePackets = pipe->Write(writePackets);
        if (!writePackets.empty())
        {
            int a = 0;
            a++;
        }
        //writePackets2 = pipe2->Write(writePackets2);
        //pipe2->Write(writeData);
        //pipe3->Write(writeData);
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
