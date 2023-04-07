#pragma once

#include <chrono>
#include <stop_token>

namespace FastTransport::Protocol {
class UDPQueue;
class Socket;

class SendThreadQueue {
    using clock = std::chrono::steady_clock;

public:
    SendThreadQueue();

    static void WriteThread(std::stop_token stop, UDPQueue& udpQueue, SendThreadQueue& sendThreadQueue, const Socket& socket, size_t index);

private:
    std::chrono::milliseconds _pauseTime;
};
} // namespace FastTransport::Protocol
