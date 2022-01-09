#include "FastTransportProtocol.hpp"

#include "PeriodicExecutor.hpp"

namespace FastTransport::Protocol {

static IPacket::List Recv()
{
    IPacket::List result;

    return result;
}

FastTransportContext::FastTransportContext(int port)
    : _shutdownContext(false)
    , _udpQueue(port, 2, 1000, 100)
    , _sendContextThread(SendThread, std::ref(*this))
    , _recvContextThread(RecvThread, std::ref(*this))
{
    for (int i = 0; i < 1000; i++) {
        _freeRecvPackets.push_back(std::move(std::make_unique<Packet>(1500)));
    }
    _udpQueue.Init();
}

FastTransportContext::~FastTransportContext()
{
    _shutdownContext = true;

    _sendContextThread.join();
    _recvContextThread.join();
}

IConnection* FastTransportContext::Accept()
{
    if (_incomingConnections.empty()) {
        return nullptr;
    }

    Connection* connection = _incomingConnections.back();
    _incomingConnections.pop_back();

    _connections.insert({ connection->GetConnectionKey(), connection });

    return connection;
}

IConnection* FastTransportContext::Connect(const ConnectionAddr& dstAddr)
{
    auto* connection = new Connection(new SendingSynState(), dstAddr, GenerateID()); // NOLINT(cppcoreguidelines-owning-memory)
    _connections.insert({ connection->GetConnectionKey(), connection });

    return connection;
}

IPacket::PairList FastTransportContext::OnReceive(IPacket::List&& packets)
{
    IPacket::PairList freePackets;

    for (auto& packet : packets) {
        auto packets = OnReceive(std::move(packet));
        freePackets.first.splice(std::move(packets.first));
        freePackets.second.splice(std::move(packets.second));
    }

    return freePackets;
}

IPacket::PairList FastTransportContext::OnReceive(IPacket::Ptr&& packet)
{
    IPacket::PairList freePackets;

    Header header = packet->GetHeader();
    if (!header.IsValid()) {
        throw std::runtime_error("Not implemented");
    }

    auto connection = _connections.find(ConnectionKey(packet->GetDstAddr(), packet->GetHeader().GetDstConnectionID()));

    if (connection != _connections.end()) {
        auto freeRecvPackets = connection->second->OnRecvPackets(std::move(packet));
        freePackets.first.splice(std::move(freeRecvPackets.first));
        freePackets.second.splice(std::move(freeRecvPackets.second));
    } else {
        auto [connection, freeRecvPackets] = _listen.Listen(std::move(packet), GenerateID());
        if (connection != nullptr) {
            _incomingConnections.push_back(connection);
            _connections.insert({ connection->GetConnectionKey(), connection });
        }

        freePackets.first.splice(std::move(freeRecvPackets));
    }

    return freePackets;
}

void FastTransportContext::ConnectionsRun()
{
    for (auto& [key, connection] : _connections) {
        connection->Run();
    }

    SendQueueStep();
}

void FastTransportContext::SendQueueStep()
{
    OutgoingPacket::List packets;
    for (auto& [key, connection] : _connections) {
        packets.splice(connection->GetPacketsToSend());
    }

    OutgoingPacket::List inFlightPackets = Send(packets);

    std::unordered_map<ConnectionKey, OutgoingPacket::List> connectionOutgoingPackets;
    for (auto& outgoingPacket : inFlightPackets) {
        auto& packet = outgoingPacket._packet;
        ConnectionKey key(packet->GetDstAddr(), packet->GetHeader().GetSrcConnectionID());

        connectionOutgoingPackets[key].push_back(std::move(outgoingPacket));
    }

    for (auto& [connectionKey, outgoingPacket] : connectionOutgoingPackets) {
        auto connection = _connections.find(connectionKey);

        if (connection != _connections.end()) {
            connection->second->ProcessSentPackets(std::move(outgoingPacket));
        } else {
            // needs return free packets to pool
            throw std::runtime_error("Not implemented");
        }
    }
}

void FastTransportContext::RecvQueueStep()
{
    // TODO: get 1k freePackets
    IPacket::List freePackets;
    {
        std::lock_guard lock(_freeRecvPackets._mutex);
        freePackets.splice(std::move(_freeRecvPackets));
    }

    auto receivedPackets = _udpQueue.Recv(std::move(freePackets));

    auto packets = OnReceive(std::move(receivedPackets));

    {
        std::lock_guard lock(_freeRecvPackets._mutex);
        _freeRecvPackets.splice(std::move(packets.first));
    }

    {
        std::lock_guard lock(_freeSendPackets._mutex);
        _freeSendPackets.splice(std::move(packets.second));
    }
}

void FastTransportContext::CheckRecvQueue()
{
    for (auto& [key, connection] : _connections) {
        connection->ProcessRecvPackets();
    }
}

OutgoingPacket::List FastTransportContext::Send(OutgoingPacket::List& packets)
{
    if (packets.empty())
        return OutgoingPacket::List();

    return _udpQueue.Send(std::move(packets));
}

void FastTransportContext::SendThread(FastTransportContext& context)
{
    PeriodicExecutor pe([&context]() {
        context.ConnectionsRun();
    },
        20ms);

    while (!context._shutdownContext) {
        pe.Run();
    }
}

void FastTransportContext::RecvThread(FastTransportContext& context)
{
    PeriodicExecutor pe([&context]() {
        context.RecvQueueStep();
        context.CheckRecvQueue();
    },
        20ms);

    while (!context._shutdownContext) {
        pe.Run();
    }
}

} // namespace FastTransport::Protocol
