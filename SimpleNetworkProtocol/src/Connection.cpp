#include "Connection.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <utility>

#include "ConnectionKey.hpp"
#include "ConnectionState.hpp"
#include "HeaderTypes.hpp"
#include "IConnectionState.hpp"
#include "IInFlightQueue.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "ISendQueue.hpp"
#include "IStatistics.hpp"
#include "InFlightQueue.hpp"
#include "OutgoingPacket.hpp"
#include "RecvQueue.hpp"
#include "RecvQueueStatus.hpp"
#include "SendQueue.hpp"
#include "Statistics.hpp"

namespace FastTransport::Protocol {
class ConnectionAddr;
} // namespace FastTransport::Protocol

namespace FastTransport::Protocol {

Connection::Connection(ConnectionState state, const ConnectionAddr& addr, ConnectionID myID)
    : _context(std::make_shared<ConnectionContext>())
    , _key(addr, myID)
    , _lastPacketReceive(clock::now())
    , _inFlightQueue(std::make_unique<InFlightQueue>(_context))
    , _recvQueue(std::make_unique<RecvQueue>())
    , _sendQueue(std::make_unique<SendQueue>())
    , _closed(false)
    , _cleanRecvBuffers(false)
    , _cleanSendBuffers(false)
    , _connectionState(state)
{

    _states.emplace(ConnectionState::ClosedState, std::make_unique<ClosedState>());
    _states.emplace(ConnectionState::ClosingState, std::make_unique<ClosingState>());
    _states.emplace(ConnectionState::DataState, std::make_unique<DataState>());
    _states.emplace(ConnectionState::SendingSynAckState, std::make_unique<SendingSynAckState>());
    _states.emplace(ConnectionState::SendingSynState, std::make_unique<SendingSynState>());
    _states.emplace(ConnectionState::WaitingSynAckState, std::make_unique<WaitingSynAckState>());
    _states.emplace(ConnectionState::WaitingSynState, std::make_unique<WaitingSynState>());
}

Connection ::~Connection() = default;

IPacket::List Connection::OnRecvPackets(IPacket::Ptr&& packet)
{
    _lastPacketReceive = clock::now();
    auto [connectionState, freePackets] = _states[_connectionState]->OnRecvPackets(std::move(packet), *this);

    _connectionState = connectionState;

    IPacket::List temp;
    _freeRecvPackets.LockedSwap(temp);
    freePackets.splice(std::move(temp));

    return std::move(freePackets);
}

bool Connection::IsConnected() const
{
    return _connected;
}

void Connection::SetConnected(bool connected)
{
    _connected = connected;
}

const IStatistics& Connection::GetStatistics() const
{
    return _statistics;
}

Statistics& Connection::GetStatistics()
{
    return _statistics;
}

ConnectionContext& Connection::GetContext()
{
    return *_context;
}

IPacket::List Connection::Send(std::stop_token stop, IPacket::List&& data)
{
    if (!data.empty()) {
        _sendUserData.LockedSplice(std::move(data));
    }

    IPacket::List result;
    {
        if (_freeUserSendPackets.Wait(stop, [this]() { return IsClosed(); })) {
            _freeUserSendPackets.LockedSwap(result);
        }
    }

    return result;
}

IPacket::List Connection::Recv(std::stop_token stop, IPacket::List&& freePackets)
{
    if (!freePackets.empty()) {
        _freeRecvPackets.LockedSplice(std::move(freePackets));
    }

    IPacket::List result;
    {
        LockedList<IPacket::Ptr>& userData = _recvQueue->GetUserData();
        if (userData.Wait(stop, [this]() { return IsClosed(); })) {
            userData.LockedSwap(result);
        }
    }

    return result;
}

void Connection::AddFreeRecvPackets(IPacket::List&& freePackets)
{
    if (!freePackets.empty()) {
        for (auto& packet : freePackets) {
            std::array<std::byte, 1300> payload {};
            packet->SetPayload(payload);
        }
        _freeRecvPackets.LockedSplice(std::move(freePackets));
    }
}

void Connection::AddFreeSendPackets(IPacket::List&& freePackets)
{
    for (auto& packet : freePackets) {
        std::array<std::byte, 1300> payload {};
        packet->SetPayload(payload);
    }
    _freeUserSendPackets.LockedSplice(std::move(freePackets));
}

void Connection::Close()
{
    _closed = true;
    _freeUserSendPackets.NotifyAll();
    _recvQueue->GetUserData().NotifyAll();
}

bool Connection::IsClosed() const
{
    return _closed;
}

bool Connection::CanBeDeleted() const
{
    return _cleanRecvBuffers && _cleanSendBuffers;
}

const ConnectionKey& Connection::GetConnectionKey() const
{
    return _key;
}

OutgoingPacket::List Connection::GetPacketsToSend()
{
    const std::size_t size = _inFlightQueue->GetNumberPacketToSend();
    OutgoingPacket::List packets = _sendQueue->GetPacketsToSend(size);
    _statistics.AddSendPackets(packets.size());
    return packets;
}

void Connection::SetInternalFreePackets(IPacket::List&& freeInternalSendPackets, IPacket::List&& freeRecvPackets)
{
    _freeInternalSendPackets.splice(std::move(freeInternalSendPackets));
    _freeRecvPackets.LockedSplice(std::move(freeRecvPackets));
}

void Connection::ProcessSentPackets(OutgoingPacket::List&& packets)
{
    auto [freeInternalPackets, freePackets] = _inFlightQueue->AddQueue(std::move(packets));

    _freeInternalSendPackets.splice(std::move(freeInternalPackets));

    if (!freePackets.empty()) {
        _freeUserSendPackets.LockedSplice(std::move(freePackets));
        _freeUserSendPackets.NotifyAll();
    }
}

void Connection::ProcessRecvPackets()
{
    if (IsClosed()) {
        return;
    }

    _recvQueue->ProccessUnorderedPackets();
}

void Connection::SendServicePackets(IPacket::List&& packets)
{
    _statistics.AddAckSendPackets(packets.size());
    _sendQueue->SendPackets(std::move(packets), false);
}

void Connection::SendDataPackets(IPacket::List&& packets)
{
    _sendQueue->SendPackets(std::move(packets), true);
}

IPacket::Ptr Connection::RecvPacket(IPacket::Ptr&& packet)
{
    auto [status, freePacket] = _recvQueue->AddPacket(std::move(packet));
    switch (status) {
    case RecvQueueStatus::Full: {
        _statistics.AddOverflowPackets();
        break;
    }
    case RecvQueueStatus::Duplicated: {
        _statistics.AddDuplicatePackets();
        break;
    }
    case RecvQueueStatus::New: {
        _statistics.AddReceivedPackets();
        break;
    }
    default: {
        throw std::runtime_error("Unknown RecvQueueStatus");
    }
    }
    return std::move(freePacket);
}

void Connection::ReSendPackets(OutgoingPacket::List&& packets)
{
    _statistics.AddLostPackets(packets.size());
    _sendQueue->ReSendPackets(std::move(packets));
}

IPacket::List Connection::ProcessAcks()
{
    return _inFlightQueue->ProcessAcks();
}

OutgoingPacket::List Connection::CheckTimeouts()
{
    return _inFlightQueue->CheckTimeouts();
}

void Connection::AddAcks(std::span<SeqNumberType> acks)
{
    _statistics.AddAckReceivedPackets();
    return _inFlightQueue->AddAcks(acks);
}

void Connection::SetLastAck(SeqNumberType acks)
{
    return _inFlightQueue->SetLastAck(acks);
}

IRecvQueue& Connection::GetRecvQueue()
{
    return *_recvQueue;
}

IPacket::List Connection::GetSendUserData()
{
    IPacket::List result;
    _sendUserData.LockedSwap(result);
    return result;
}

void Connection::SetDestinationID(ConnectionID destinationId)
{
    _destinationId = destinationId;
}

ConnectionID Connection::GetDestinationID() const
{
    return _destinationId;
}

IPacket::List Connection::GetFreeRecvPackets()
{
    IPacket::List freePackets;
    IPacket::List temp;
    _freeRecvPackets.LockedSwap(temp);
    freePackets.splice(std::move(temp));

    return freePackets;
}

IPacket::List Connection::CleanFreeRecvPackets()
{
    _cleanRecvBuffers = true;
    return GetFreeRecvPackets();
}

IPacket::List Connection::CleanFreeSendPackets()
{
    _cleanSendBuffers = true;
    IPacket::List freePackets = _inFlightQueue->GetAllPackets();

    return freePackets;
}

IPacket::Ptr Connection::GetFreeSendPacket()
{
    IPacket::Ptr packet = std::move(_freeInternalSendPackets.back());
    _freeInternalSendPackets.pop_back();
    return packet;
}

void Connection::AddFreeUserSendPackets(IPacket::List&& freePackets)
{
    if (!freePackets.empty()) {
        _freeUserSendPackets.LockedSplice(std::move(freePackets));
        _freeUserSendPackets.NotifyAll();
    }
}

void Connection::Run()
{
    const auto& connectionState = _states[_connectionState];
    if (clock::now() - _lastPacketReceive > connectionState->GetTimeout()) {
        _connectionState = connectionState->OnTimeOut(*this);
        _lastPacketReceive = clock::now();
    } else {
        _connectionState = connectionState->SendPackets(*this);
        connectionState->ProcessInflightPackets(*this);
    }
}
} // namespace FastTransport::Protocol
