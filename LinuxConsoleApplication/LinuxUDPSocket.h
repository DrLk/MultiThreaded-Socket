#pragma once

#include<vector>
#include<list>
#include<thread>
#include<mutex>

#include "ReadThreadQueue.h"

class Packet;

class LinuxUDPSocket
{
public:
    LinuxUDPSocket(int port, int sendThreadCount, int recvThreadCount, int sendQueueSizePerThreadCount, int recvQueueSizePerThreadCount);
    ~LinuxUDPSocket();
    void Init();

    std::list<Packet*> Recv(std::list<Packet*>&& freeBuffers);
    std::list<Packet*> Send(std::list<Packet*>&& data);

    std::list<Packet*> CreateBuffers(int size);

private:
    uint _socket;
    uint _port;
    std::vector<std::thread> _writeThreads;
    std::vector<std::thread> _readThreads;

    std::list<Packet*> _sendQueue;
    std::list<Packet*> _recvQueue;

    std::mutex _sendQueueMutex;
    std::mutex _recvQueueMutex;

    std::list<Packet*> _sendFreeQueue;
    std::list<Packet*> _recvFreeQueue;

    std::mutex _sendFreeQueueMutex;
    std::mutex _recvFreeQueueMutex;

    static void WriteThread(LinuxUDPSocket& socket);
    static void ReadThread(LinuxUDPSocket& socket, RecvThreadQueue& recvThreadQueue);

    std::vector<RecvThreadQueue*> _recvThreadQueues;

    int _writeThreadCount;
    int _readThreadCount;

    int _sendQueueSizePerThread;
    int _recvQueueSizePerThread;
};
