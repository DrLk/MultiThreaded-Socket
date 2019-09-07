#include "SelectPipe.h"
#include <stdio.h>
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <thread>

#include "SelectPacket.h"
#include "SelectWriteThread.h"

#pragma comment(lib, "Ws2_32.lib")

#define PACKET_SIZE 1000

SelectPipe::SelectPipe()
{
    writeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    readEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    //readEvent = WSACreateEvent();


}

#define DATA_BUFSIZE 8192

typedef struct _SOCKET_INFORMATION
{
    CHAR Buffer[DATA_BUFSIZE];
    WSABUF DataBuf;
    SOCKET Socket;
    OVERLAPPED Overlapped;
    DWORD BytesSEND;
    DWORD BytesRECV;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

//LPSOCKET_INFORMATION SocketArray[WSA_MAXIMUM_WAIT_EVENTS];
//WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
DWORD EventTotal = 0;

BOOL CreateSocketInformation(SOCKET s)

{
    LPSOCKET_INFORMATION SI;

    
    if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
        printf("GlobalAlloc() failed with error %d\n", GetLastError());
        return FALSE;
    }
    else
        printf("GlobalAlloc() for LPSOCKET_INFORMATION is OK!\n");

    // Prepare SocketInfo structure for use
    SI->Socket = s;
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;

    EventTotal++;
    return(TRUE);
}


void SelectPipe::Listen(int port)
{
    int result = 0;
    if ((result = WSAStartup(0x0202, &wsaData)) != 0)
    {
        printf("WSAStartup() failed with error %d\n", result);
        WSACleanup();
        throw std::exception();
    }
    else
    {
        printf("WSAStartup() is fine!\n");
    }

    if ((listenSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)

    {
        printf("socket() failed with error %d\n", WSAGetLastError());
        return;
    }
    else
        printf("socket() is OK!\n");

    //char one = 1;
    //auto r = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    //char val = 1;
    //r = setsockopt(listenSocket, IPPROTO_IP, IP_DONTFRAGMENT, &val, sizeof(val));


    if (CreateSocketInformation(listenSocket) == FALSE)
        printf("CreateSocketInformation() failed!\n");
    else
        printf("CreateSocketInformation() is OK lol!\n");

    /*if (WSAEventSelect(listenSocket, readEvent, FD_READ) == SOCKET_ERROR)
    {
        printf("WSASocket() failed with error %d\n", WSAGetLastError());
        throw std::exception();
    }
    else
        printf("WSASocket() is OK!\n");*/

    /*if (WSAEventSelect(listenSocket, writeEvent, FD_WRITE) == SOCKET_ERROR)
    {
        printf("WSASocket() failed with error %d\n", WSAGetLastError());
        throw std::exception();
    }
    else
        printf("WSASocket() is OK!\n");*/


    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    InternetAddr.sin_port = htons(port);


    if (bind(listenSocket, (PSOCKADDR)& InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
    {
        printf("bind() failed with error %d\n", WSAGetLastError());
        throw std::exception();
    }
    else
    {
        printf("bind() is OK!\n");
    }


    /*if (listen(listenSocket, 5))
    {
        printf("listen() failed with error %d\n", WSAGetLastError());
        throw std::exception();
    }
    else
    {
        printf("listen() is OK!\n");
    }*/



    /*for (int i = 0; i < writeThreadCount; i++)
    {
        SelectWriteThread* writeThread = new SelectWriteThread(*this); //TODO: store write thread
        writeThreads.push_back(std::thread(SelectPipe::WriteThread, std::ref(*writeThread)));
    }*/

    for (int i = 0; i < readThreadCount; i++)
    {
        readThreads.push_back(std::thread(SelectPipe::ReadThread, std::ref(*this)));
    }
}

void SelectPipe::WriteThread(SelectWriteThread& writeThread)
{
    SelectPipe& pipe = writeThread.GetPipe();
    while (true)
    {
        WaitForSingleObject(pipe.writeEvent, INFINITE);
        writeThread.WriteSocket();
    }

}

void SelectPipe::ReadThread(SelectPipe& pipe)
{
    HANDLE readEvent = pipe.readEvent;
    DWORD event;
    while (true)
    {
        // Wait for one of the sockets to receive I/O notification and
        /*if ((event = WSAWaitForMultipleEvents(1, &readEvent, FALSE, INFINITE, FALSE)) == WSA_WAIT_FAILED)
        {
            printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
            return;
        }
        else
            printf("WSAWaitForMultipleEvents() is pretty damn OK!\n");

        int index = event - WSA_WAIT_EVENT_0;

        if (event == WSA_WAIT_EVENT_0)*/
        {
            pipe.ReadSocket();
        }
        /*if (WSAEventSelect(pipe.listenSocket, readEvent, FD_READ) == SOCKET_ERROR)
        {
            printf("WSASocket() failed with error %d\n", WSAGetLastError());
            throw std::exception();
        }
        else
            printf("WSASocket() is OK!\n");*/
    }

}

std::list<IPacket*> SelectPipe::Read(std::list<IPacket*> buffers)
{
    std::list<IPacket*> result;
    {
        std::lock_guard<std::mutex> lock(freeReadQueueMutex);
        freeReadQueue.splice(freeReadQueue.end(), buffers);
    }
    {
        std::lock_guard<std::mutex> lock(readQueueMutex);
        result.swap(readQueue);
    }
    return  result;
}

std::list<IPacket*> SelectPipe::Write(std::list<IPacket*>& data)
{
    if (!data.empty())
    {
        std::lock_guard<std::mutex> lock(writeQueueMutex);
        {
            writeQueue.splice(writeQueue.end(), data);
        }

        SetEvent(writeEvent);
    }

    std::list<IPacket*> result;
    {
        std::lock_guard<std::mutex> lock(freeWriteQueueMutex);
        result.swap(freeWriteQueue);
    }
    return result;
}

std::list<IPacket*> SelectPipe::CreatePackets(size_t size, size_t count) const
{
    std::list<IPacket*> packets;
    for (size_t i = 0; i < count; i++)
    {
        packets.push_back(new SelectPacket(size));
    }

    return packets;
}


void SelectPipe::ReadSocket()
{
    //SockAddr srcAddr;
    //SockAddr dstAddr;
    std::vector<char> buffer(9999); // max UDP packet size
    buffer.resize(9999);
    int n = Recv(buffer);
    if (n > 0)
    {
        IPacket* packet = new SelectPacket(PACKET_SIZE);
        packet->GetBuffer() = buffer;
        {
            std::lock_guard<std::mutex> lock(readQueueMutex);
            readQueue.push_back(packet);
        }


        printf("Recv data");
        //if (!ProcessReceivedData(srcAddr, dstAddr, buf, n))
            //break;
    }
    else
    {
        Sleep(1);
        //printf("No data");
    }
}


int SelectPipe::Recv(std::vector<char>& buffer)
{
    int nRead = -1;

    sockaddr* srcAddr = nullptr;
    int len = 0;
    nRead = recvfrom(listenSocket, buffer.data(), buffer.size(), 0, srcAddr, &len);

    return nRead;
}