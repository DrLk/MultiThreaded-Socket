#pragma once

#include <chrono>



namespace FastTransport::Protocol
{
    class UDPQueue;
    class Socket;

    class SendThreadQueue
    {
        typedef std::chrono::steady_clock clock;

    public:
        SendThreadQueue();

        static void WriteThread(UDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, unsigned short index);

    private:
        std::chrono::milliseconds _pauseTime;
    };
}