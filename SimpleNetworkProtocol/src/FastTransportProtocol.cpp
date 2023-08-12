#include "FastTransportProtocol.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "IConnectionState.hpp"
#include "Logger.hpp"
#include "MultiList.hpp"
#include "ThreadName.hpp"

namespace FastTransport {
namespace Protocol {
    class ConnectionAddr;
} // namespace Protocol
} // namespace FastTransport

#define TRACER() LOGGER() << "[" << connection->GetConnectionKey() << "]: " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::Protocol {

FastTransportContext::FastTransportContext(const ConnectionAddr& address)
    : _udpQueue(address, 4, 1000, 1000)
    , _sendContextThread(SendThread, std::ref(*this))
    , _recvContextThread(RecvThread, std::ref(*this))
{
    _udpQueue.Init();
}

IConnection::Ptr FastTransportContext::Accept(std::stop_token stop)
{
    Connection::Ptr connection = nullptr;
    {
        if (_incomingConnections.Wait(stop)) {

            connection = _incomingConnections.LockedGetBack();
        } else {
            return connection;
        }
    }

    {
        const std::scoped_lock lock(_connectionsMutex);
        _connections.insert({ connection->GetConnectionKey(), connection });
    }

    return connection;
}

IConnection::Ptr FastTransportContext::Connect(const ConnectionAddr& dstAddr)
{
    const Connection::Ptr& connection = std::make_shared<Connection>(new SendingSynState(), dstAddr, GenerateID()); // NOLINT(cppcoreguidelines-owning-memory)
    connection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));

    {
        const std::scoped_lock lock(_connectionsMutex);
        _connections.insert({ connection->GetConnectionKey(), connection });
    }

    return connection;
}

void FastTransportContext::InitRecvPackets()
{
    _freeRecvPackets = UDPQueue::CreateBuffers(_udpQueue.GetRecvQueueSizePerThread() * _udpQueue.GetThreadCount());
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

    if (!packet->IsValid()) {
        throw std::runtime_error("Not implemented");
    }

    {
        std::shared_lock sharedLock(_connectionsMutex);
        auto connection = _connections.find(ConnectionKey(packet->GetDstAddr(), packet->GetDstConnectionID()));

        if (connection != _connections.end()) {
            auto freeRecvPackets = connection->second->OnRecvPackets(std::move(packet));
            freePackets.splice(std::move(freeRecvPackets));
        } else {
            auto [connection, freeRecvPackets] = ListenState::Listen(std::move(packet), GenerateID());
            if (connection != nullptr) {
                connection->SetInternalFreePackets(UDPQueue::CreateBuffers(10000), UDPQueue::CreateBuffers(10000));

                {
                    sharedLock.unlock();
                    const std::scoped_lock lock(_connectionsMutex);
                    _connections.insert({ connection->GetConnectionKey(), connection });
                }

                {
                    _incomingConnections.LockedPushBack(std::move(connection));
                    _incomingConnections.NotifyAll();
                }
            }

            freePackets.splice(std::move(freeRecvPackets));
        }
    }

    return freePackets;
}

void FastTransportContext::ConnectionsRun(std::stop_token stop)
{
    {
        const std::shared_lock lock(_connectionsMutex);

        for (auto& [key, connection] : _connections) {
            connection->Run();
        }
    }

    SendQueueStep(stop);
}

void FastTransportContext::SendQueueStep(std::stop_token stop)
{
    OutgoingPacket::List packets;

    {
        const std::shared_lock lock(_connectionsMutex);

        for (auto& [key, connection] : _connections) {
            packets.splice(connection->GetPacketsToSend());
        }
    }

    OutgoingPacket::List inFlightPackets = Send(stop, packets);

    std::unordered_map<ConnectionKey, OutgoingPacket::List, ConnectionKey::HashFunction> connectionOutgoingPackets;
    for (auto& outgoingPacket : inFlightPackets) {
        auto& packet = outgoingPacket._packet;
        const ConnectionKey key(packet->GetDstAddr(), packet->GetSrcConnectionID());

        connectionOutgoingPackets[key].push_back(std::move(outgoingPacket));
    }

    for (auto& [connectionKey, outgoingPackets] : connectionOutgoingPackets) {
        const std::shared_lock lock(_connectionsMutex);
        auto connection = _connections.find(connectionKey);

        if (connection != _connections.end()) {
            connection->second->ProcessSentPackets(std::move(outgoingPackets));
        } else {
            for (OutgoingPacket& packet : outgoingPackets) {
                _freeSendPackets.push_back(std::move(packet._packet));
            };
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
    const std::shared_lock lock(_connectionsMutex);

    for (auto& [key, connection] : _connections) {
        connection->ProcessRecvPackets();
    }
}

void FastTransportContext::RemoveRecvClosedConnections()
{
    const std::scoped_lock lock(_connectionsMutex);

    std::erase_if(_connections, [this](const std::pair<ConnectionKey, Connection::Ptr>& that) {
        const auto& [key, connection] = that;

        if (connection->IsClosed()) {
            TRACER() << "RemoveRecvClosedConnections - Closed";
            IPacket::List freeRecvPackets = connection->CleanFreeRecvPackets();
            _freeRecvPackets.splice(std::move(freeRecvPackets));
        }

        return connection->CanBeDeleted();
    });
}
void FastTransportContext::RemoveSendClosedConnections()
{
    const std::scoped_lock lock(_connectionsMutex);
    std::erase_if(_connections, [this](const std::pair<ConnectionKey, Connection::Ptr>& that) {
        const auto& [key, connection] = that;

        if (connection->IsClosed()) {
            TRACER() << "RemoveSendClosedConnections - Closed";
            IPacket::List freeSendPackets = connection->CleanFreeSendPackets();
            _freeSendPackets.splice(std::move(freeSendPackets));
        }

        return connection->CanBeDeleted();
    });
}

OutgoingPacket::List FastTransportContext::Send(std::stop_token stop, OutgoingPacket::List& packets)
{
    return _udpQueue.Send(stop, std::move(packets));
}

void FastTransportContext::SendThread(std::stop_token stop, FastTransportContext& context)
{
    SetThreadName("SendThread");

    while (!stop.stop_requested()) {
        context.RemoveSendClosedConnections();
        context.ConnectionsRun(stop);
    }
}

void FastTransportContext::RecvThread(std::stop_token stop, FastTransportContext& context)
{
    SetThreadName("RecvThread");

    context.InitRecvPackets();

    while (!stop.stop_requested()) {
        context.RemoveRecvClosedConnections();
        context.RecvQueueStep(stop);
        context.CheckRecvQueue();
    }
}

IPacket::List FastTransportContext::GetConnectionsFreeRecvPackets()
{
    const std::shared_lock lock(_connectionsMutex);

    IPacket::List freeRecvPackets;
    for (auto& [key, connection] : _connections) {
        freeRecvPackets.splice(connection->GetFreeRecvPackets());
    }

    return freeRecvPackets;
}

ConnectionID FastTransportContext::GenerateID()
{
    static std::atomic<ConnectionID> _nextID = 0;
    // TODO: Check after overflow
    return ++_nextID;
}

} // namespace FastTransport::Protocol
