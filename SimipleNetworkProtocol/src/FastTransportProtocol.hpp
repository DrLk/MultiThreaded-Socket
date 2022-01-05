#pragma once

#include <functional>
#include <memory>
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
    ~FastTransportContext();

    IPacket::PairList OnReceive(IPacket::List&& packet);

    IConnection* Accept();
    IConnection* Connect(const ConnectionAddr&);

    ConnectionID GenerateID()
    {
        static ConnectionID _nextID = 0;
        // TODO: Check after overflow
        return ++_nextID;
    }

private:
    LockedList<IPacket::Ptr> _freeSendPackets;
    LockedList<IPacket::Ptr> _freeRecvPackets;

    ListenState _listen;
    std::unordered_map<ConnectionKey, Connection*> _connections;
    std::vector<Connection*> _incomingConnections;

    OutgoingPacket::List Send(OutgoingPacket::List& packets);

    UDPQueue _udpQueue;

    std::thread _sendContextThread;
    std::thread _recvContextThread;

    std::atomic<bool> _shutdownContext;

    IPacket::PairList OnReceive(IPacket::Ptr&& packet);

    static void SendThread(FastTransportContext& context);
    static void RecvThread(FastTransportContext& context);

    void ConnectionsRun();
    void SendQueueStep();
    void RecvQueueStep();
    void CheckRecvQueue();
};
} // namespace FastTransport::Protocol