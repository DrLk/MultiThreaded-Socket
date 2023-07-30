#include "Connection.hpp"

#include "IConnectionState.hpp"
#include "InFlightQueue.hpp"
#include "RecvQueue.hpp"
#include "SendQueue.hpp"

namespace FastTransport::Protocol {

Connection::Connection(IConnectionState* state, const ConnectionAddr& addr, ConnectionID myID)
    : _state(state)
    , _key(addr, myID)
    , _lastPacketReceive(clock::now())
    , _inFlightQueue(std::make_unique<InFlightQueue>())
    , _recvQueue(std::make_unique<RecvQueue>())
    , _sendQueue(std::make_unique<SendQueue>())
    , _connected(false)
    , _closed(false)
    , _cleanRecvBuffers(false)
    , _cleanSendBuffers(false)
{
}

Connection ::~Connection() = default;

IPacket::List Connection::OnRecvPackets(IPacket::Ptr&& packet)
{
    _lastPacketReceive = clock::now();
    IPacket::List freePackets = _state->OnRecvPackets(std::move(packet), *this);

    {
        IPacket::List temp;
        _freeRecvPackets.LockedSwap(temp);
        freePackets.splice(std::move(temp));
    }

    return freePackets;
}

IPacket::List Connection::Send(std::stop_token stop, IPacket::List&& data)
{
    {
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
    {
        _freeRecvPackets.LockedSplice(std::move(freePackets));
    }

    IPacket::List result;
    {
        LockedList<IPacket::Ptr>& userData = _recvQueue->GetUserData();
        if (userData.Wait(stop, [this, &userData]() { return IsClosed(); })) {
            userData.LockedSwap(result);
        }
    }

    return result;
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
    return _sendQueue->GetPacketsToSend(size);
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

void Connection::SendPacket(IPacket::Ptr&& packet, bool needAck)
{
    _sendQueue->SendPacket(std::move(packet), needAck);
}

void Connection::ReSendPackets(OutgoingPacket::List&& packets)
{
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
    return _inFlightQueue->AddAcks(acks);
}

IRecvQueue& Connection::GetRecvQueue()
{
    return *_recvQueue;
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

void Connection::AddFreeUserSendPackets(IPacket::List&& freePackets)
{
    if (!freePackets.empty()) {
        _freeUserSendPackets.LockedSplice(std::move(freePackets));

        _freeUserSendPackets.NotifyAll();
    }
}

void Connection::Run()
{
    if (clock::now() - _lastPacketReceive > DefaultTimeOut) {
        _state = _state->OnTimeOut(*this);
        _lastPacketReceive = clock::now();
    } else {
        _state = _state->SendPackets(*this);
        _state->ProcessInflightPackets(*this);
    }
}
} // namespace FastTransport::Protocol
