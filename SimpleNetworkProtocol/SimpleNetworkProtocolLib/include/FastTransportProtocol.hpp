#pragma once

#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>

#include "Connection.hpp"
#include "ConnectionEvents.hpp"
#include "ConnectionKey.hpp"
#include "HeaderTypes.hpp"
#include "IPacket.hpp"
#include "OutgoingPacket.hpp"
#include "SpinLock.hpp"
#include "UDPQueue.hpp"
#include "UDPQueueEvents.hpp"

namespace FastTransport::Protocol {
class ConnectionAddr;
} // namespace FastTransport::Protocol

namespace FastTransport::Protocol {

class FastTransportContext : public ConnectionEvents, public UDPQueueEvents {
public:
    explicit FastTransportContext(const ConnectionAddr& address);
    FastTransportContext(const FastTransportContext& that) = delete;
    FastTransportContext(FastTransportContext&& that) = delete;
    FastTransportContext& operator=(const FastTransportContext& that) = delete;
    FastTransportContext& operator=(FastTransportContext&& that) = delete;
    ~FastTransportContext() override = default;

    IPacket::List OnReceive(IPacket::List&& packet);

    IConnection::Ptr Accept(std::stop_token stop);
    IConnection::Ptr Connect(const ConnectionAddr& dstAddr);

private:
    IPacket::List _freeRecvPackets;
    IPacket::List _freeSendPackets;

    std::shared_mutex _connectionsMutex;
    std::unordered_map<ConnectionKey, std::shared_ptr<Connection>, ConnectionKey::HashFunction> _connections;
    LockedList<Connection::Ptr> _incomingConnections;

    OutgoingPacket::List Send(std::stop_token stop, OutgoingPacket::List& packets);

    UDPQueue _udpQueue;

    std::atomic_bool _readySend { true };
    using Mutex = FastTransport::Thread::SpinLock;
    mutable Mutex _mutex;
    std::condition_variable_any _condition;

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

    void OnSendPacket() override;
    void OnOutgoingPackets() override;
};
} // namespace FastTransport::Protocol
