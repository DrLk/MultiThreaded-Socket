#pragma once
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include<string.h> //memset

#include<mutex>
#include<list>
#include<vector>
#include <exception>

#include "Packet.h"


class RecvThreadQueue
{ 
public:
    RecvThreadQueue(uint port) : _port(port)
    {
    }

    ~RecvThreadQueue()
    {
        close(_socket);
    }

    RecvThreadQueue(const RecvThreadQueue&) = delete;
    RecvThreadQueue& operator=(const RecvThreadQueue&) = delete;

    void Init()
    {
        struct sockaddr_in si_me;

        //create a UDP socket
        if ((_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            throw std::runtime_error("socket");
        }

        bool on = 1;
        if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
            throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

        // zero out the structure
        memset((char*)& si_me, 0, sizeof(si_me));

        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(_port);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        //bind socket to port
        if (bind(_socket, (struct sockaddr*) & si_me, sizeof(si_me)) == -1)
        {
            throw std::runtime_error("bind");
        }

    }

    size_t RecvFrom(std::vector<char>& buffer, sockaddr* addr, socklen_t* len)
    {
        return recvfrom(_socket, buffer.data(), buffer.size(), 0, addr, len);
    }

    std::mutex _recvThreadQueueMutex;
    std::list<Packet*> _recvThreadQueue;

private:
    int _socket;
    uint _port;
};
