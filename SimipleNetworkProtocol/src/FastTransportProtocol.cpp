#include "FastTransportProtocol.hpp"

#include "Packet.hpp"
#include "PeriodicExecutor.hpp"
#include "ThreadName.hpp"

namespace FastTransport::Protocol {

FastTransportContext::FastTransportContext(const ConnectionAddr& address)
    : _udpQueue(address, 4, 1000, 1000)
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
    connection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
    _connections.insert({ connection->GetConnectionKey(), connection });

    return connection;
}

void FastTransportContext::InitRecvPackets()
{
    _freeRecvPackets = UDPQueue::CreateBuffers(1000);
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
            connection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));
            _incomingConnections.push_back(connection);
            _connections.insert({ connection->GetConnectionKey(), connection });
        }

        freePackets.splice(std::move(freeRecvPackets));
    }

    return freePackets;
}

void FastTransportContext::ConnectionsRun(std::stop_token stop)
{
    for (auto& [key, connection] : _connections) {
        connection->Run();
    }

    SendQueueStep(stop);
}

void FastTransportContext::SendQueueStep(std::stop_token stop)
{
    OutgoingPacket::List packets;
    for (auto& [key, connection] : _connections) {
        packets.splice(connection->GetPacketsToSend());
    }

    OutgoingPacket::List inFlightPackets = Send(stop, packets);

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

void FastTransportContext::RecvQueueStep(std::stop_token stop)
{
    // TODO: get 1k freePackets
    IPacket::List freePackets;
    freePackets.swap(_freeRecvPackets);

    auto receivedPackets = _udpQueue.Recv(stop, std::move(freePackets));

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

OutgoingPacket::List FastTransportContext::Send(std::stop_token stop, OutgoingPacket::List& packets)
{
    return _udpQueue.Send(stop, std::move(packets));
}

void FastTransportContext::SendThread(std::stop_token stop, FastTransportContext& context)
{
    SetThreadName("SendThread");

    while (!stop.stop_requested()) {
        context.ConnectionsRun(stop);
    }
}

void FastTransportContext::RecvThread(std::stop_token stop, FastTransportContext& context)
{
    SetThreadName("RecvThread");

    context.InitRecvPackets();

    while (!stop.stop_requested()) {
        context.RecvQueueStep(stop);
        context.CheckRecvQueue();
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
