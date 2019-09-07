#pragma once

#include "IPipe.h"

#ifdef WIN32
#include <Winsock2.h>
#include <Windows.h>
#include <MSWSock.h>
#else
#include <sys/socket.h>
#endif
#include <thread>
#include <mutex>
#include <list>

class SelectWriteThread;


class SelectPipe : public IPipe
{
public:
    SelectPipe();

    virtual std::list<IPacket*> Read(std::list<IPacket*> data) override;
    virtual std::list<IPacket*> Write(std::list<IPacket*>& data) override;
    virtual std::list<IPacket*> CreatePackets(size_t size, size_t count) const override;

    virtual void Listen(int port) override;

    ///////////////////////////////////////
    // TODO:: should be private
    std::mutex writeQueueMutex;
    std::list<IPacket*> writeQueue;
    std::mutex freeWriteQueueMutex;
    std::list<IPacket*> freeWriteQueue;

    SOCKET listenSocket;

    HANDLE writeEvent = nullptr;
/////////////////////////////////////
private:
    WSADATA wsaData;
    SOCKADDR_IN InternetAddr;
    HANDLE readEvent = nullptr;

    int writeThreadCount = 2;
    std::vector<std::thread> writeThreads;
    int readThreadCount = 1;
    std::vector<std::thread> readThreads;

    std::mutex readQueueMutex;
    std::list<IPacket*> readQueue;
    std::mutex freeReadQueueMutex;
    std::list<IPacket*> freeReadQueue;

    static void ReadThread(SelectPipe& pipe);
    static void WriteThread(SelectWriteThread& writeThread);
    void ReadSocket();
    int Recv(std::vector<char>& buffer);
};
