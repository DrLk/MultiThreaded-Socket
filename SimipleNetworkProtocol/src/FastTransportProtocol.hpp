#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include "Connection.hpp"
#include "ConnectionKey.hpp"
#include "IConnectionState.hpp"
#include "IPacket.hpp"
#include "UDPQueue.hpp"

namespace FastTransport::Protocol {

class FastTransportContext {
public:
    explicit FastTransportContext(const ConnectionAddr& address);
    FastTransportContext(const FastTransportContext& that) = delete;
    FastTransportContext(FastTransportContext&& that) = delete;
    FastTransportContext& operator=(const FastTransportContext& that) = delete;
    FastTransportContext& operator=(FastTransportContext&& that) = delete;
    ~FastTransportContext() = default;

    IPacket::List OnReceive(IPacket::List&& packet);

    IConnection::Ptr Accept(std::stop_token stop);
    IConnection::Ptr Connect(const ConnectionAddr&);

private:
    IPacket::List _freeRecvPackets;
    IPacket::List _freeSendPackets;

    std::shared_mutex _connectionsMutex;
    std::unordered_map<ConnectionKey, std::shared_ptr<Connection>, ConnectionKey::HashFunction> _connections;
    LockedList<Connection::Ptr> _incomingConnections;

    OutgoingPacket::List Send(std::stop_token stop, OutgoingPacket::List& packets);

    UDPQueue _udpQueue;

    std::jthread _sendContextThread;
    std::jthread _recvContextThread;

    void InitRecvPackets();
    IPacket::List OnReceive(IPacket::Ptr&& packet);

    static void SendThread(std::stop_token stop, FastTransportContext& context);
    static void RecvThread(std::stop_token stop, FastTransportContext& context);

    void ConnectionsRun(std::stop_token stop);
    void SendQueueStep(std::stop_token stop);
    void RecvQueueStep(std::stop_token stop);
    void CheckRecvQueue();

    void RemoveRecvClosedConnections();
    void RemoveSendClosedConnections();

    IPacket::List GetConnectionsFreeRecvPackets();

    static ConnectionID GenerateID();
};
} // namespace FastTransport::Protocol
