#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>

#include "Connection.hpp"
#include "ConnectionKey.hpp"
#include "IConnectionState.hpp"
#include "IPacket.hpp"
#include "SpeedController.hpp"
#include "UDPQueue.hpp"

namespace FastTransport::Protocol {

class FastTransportContext {
public:
    explicit FastTransportContext(int port);
    FastTransportContext(const FastTransportContext& that) = delete;
    FastTransportContext(FastTransportContext&& that) = delete;
    FastTransportContext& operator=(const FastTransportContext& that) = delete;
    FastTransportContext& operator=(FastTransportContext&& that) = delete;
    ~FastTransportContext() = default;

    IPacket::List OnReceive(IPacket::List&& packet);

    IConnection* Accept();
    IConnection* Connect(const ConnectionAddr&);

private:
    IPacket::List _freeRecvPackets;

    std::unordered_map<ConnectionKey, Connection*> _connections;
    std::vector<Connection*> _incomingConnections;

    OutgoingPacket::List Send(OutgoingPacket::List& packets);

    UDPQueue _udpQueue;

    std::jthread _sendContextThread;
    std::jthread _recvContextThread;

    void InitRecvPackets();
    IPacket::List OnReceive(IPacket::Ptr&& packet);

    static void SendThread(const std::stop_token& stop, FastTransportContext& context);
    static void RecvThread(const std::stop_token& stop, FastTransportContext& context);

    void ConnectionsRun();
    void SendQueueStep();
    void RecvQueueStep();
    void CheckRecvQueue();

    IPacket::List GetConnectionsFreeRecvPackets();

    static ConnectionID GenerateID();
};
} // namespace FastTransport::Protocol
