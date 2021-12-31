#pragma once

#include <chrono>

namespace FastTransport::Protocol {
class UDPQueue;
class Socket;

class SendThreadQueue {
    using clock = std::chrono::steady_clock;

public:
    SendThreadQueue();

    static void WriteThread(UDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, unsigned short index);

private:
    std::chrono::milliseconds _pauseTime;
};
} // namespace FastTransport::Protocol
