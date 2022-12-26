#include "FastTransportProtocol.hpp"

#include "Packet.hpp"
#include "PeriodicExecutor.hpp"

namespace FastTransport::Protocol {

FastTransportContext::FastTransportContext(int port)
    : _udpQueue(port, 2, 100, 100)
    , _sendContextThread(SendThread, std::ref(*this))
    , _recvContextThread(RecvThread, std::ref(*this))
{
    _udpQueue.Init();
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

void FastTransportContext::InitRecvPackets()
{
    for (int i = 0; i < 1000; i++) {
        _freeRecvPackets.push_back(std::move(std::make_unique<Packet>(1500)));
    }
}

IPacket::List FastTransportContext::OnReceive(IPacket::List&& packets)
{
    IPacket::List freePackets;

    for (auto& packet : packets) {
        auto packets = OnReceive(std::move(packet));
        freePackets.splice(std::move(packets));
    }

    return freePackets;
}

IPacket::List FastTransportContext::OnReceive(IPacket::Ptr&& packet)
{
    IPacket::List freePackets;

    const Header header = packet->GetHeader();
    if (!header.IsValid()) {
        throw std::runtime_error("Not implemented");
    }

    auto connection = _connections.find(ConnectionKey(packet->GetDstAddr(), packet->GetHeader().GetDstConnectionID()));

    if (connection != _connections.end()) {
        auto freeRecvPackets = connection->second->OnRecvPackets(std::move(packet));
        freePackets.splice(std::move(freeRecvPackets));
    } else {
        auto [connection, freeRecvPackets] = ListenState::Listen(std::move(packet), GenerateID());
        if (connection != nullptr) {
            _incomingConnections.push_back(connection);
            _connections.insert({ connection->GetConnectionKey(), connection });
        }

        freePackets.splice(std::move(freeRecvPackets));
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
        const ConnectionKey key(packet->GetDstAddr(), packet->GetHeader().GetSrcConnectionID());

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
    freePackets.swap(_freeRecvPackets);

    auto receivedPackets = _udpQueue.Recv(std::move(freePackets));

    auto freeRecvPackets = OnReceive(std::move(receivedPackets));

    freeRecvPackets.splice(GetConnectionsFreeRecvPackets());

    _freeRecvPackets.splice(std::move(freeRecvPackets));
}

void FastTransportContext::CheckRecvQueue()
{
    for (auto& [key, connection] : _connections) {
        connection->ProcessRecvPackets();
    }
}

OutgoingPacket::List FastTransportContext::Send(OutgoingPacket::List& packets)
{
    return _udpQueue.Send(std::move(packets));
}

void FastTransportContext::SendThread(const std::stop_token& stop, FastTransportContext& context)
{
    PeriodicExecutor executor([&context]() {
        context.ConnectionsRun();
    },
        50ms);

    while (!stop.stop_requested()) {
        executor.Run();
    }
}

void FastTransportContext::RecvThread(const std::stop_token& stop, FastTransportContext& context)
{
    PeriodicExecutor executor([&context]() {
        context.RecvQueueStep();
        context.CheckRecvQueue();
    },
        50ms);

    context.InitRecvPackets();

    while (!stop.stop_requested()) {
        executor.Run();
    }
}

IPacket::List FastTransportContext::GetConnectionsFreeRecvPackets()
{
    IPacket::List freeRecvPackets;
    for (auto& [key, connection] : _connections) {
        freeRecvPackets.splice(connection->GetFreeRecvPackets());
    }

    return freeRecvPackets;
}

ConnectionID FastTransportContext::GenerateID()
{
    static ConnectionID _nextID = 0;
    // TODO: Check after overflow
    return ++_nextID;
}

} // namespace FastTransport::Protocol
