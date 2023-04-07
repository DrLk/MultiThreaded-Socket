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
{
}

Connection ::~Connection() = default;

IPacket::List Connection::OnRecvPackets(IPacket::Ptr&& packet)
{
    _lastPacketReceive = clock::now();
    auto freePackets = _state->OnRecvPackets(std::move(packet), *this);

    {
        const std::scoped_lock lock(_freeRecvPackets._mutex);
        freePackets.splice(std::move(_freeRecvPackets));
    }

    return freePackets;
}

IPacket::List Connection::Send(std::stop_token stop, IPacket::List&& data)
{
    {
        const std::scoped_lock lock(_sendUserDataMutex);
        _sendUserData.splice(std::move(data));
    }

    IPacket::List result;
    {
        std::unique_lock lock(_freeUserSendPackets._mutex);
        _freeUserSendPackets.Wait(lock, stop, [this]() { return !_freeUserSendPackets.empty(); });
        result.swap(_freeUserSendPackets);
    }

    return result;
}

IPacket::List Connection::Recv(std::stop_token stop, IPacket::List&& freePackets)
{
    {
        const std::scoped_lock lock(_freeRecvPackets._mutex);
        _freeRecvPackets.splice(std::move(freePackets));
    }

    return _recvQueue->GetUserData(stop);
}

void Connection::Close()
{
    _closed = true;
}

bool Connection::IsClosed() const
{
    return _closed;
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
    _freeRecvPackets.splice(std::move(freeRecvPackets));
}

void Connection::ProcessSentPackets(OutgoingPacket::List&& packets)
{
    auto [freeInternalPackets, freePackets] = _inFlightQueue->AddQueue(std::move(packets));

    _freeInternalSendPackets.splice(std::move(freeInternalPackets));

    if (!freePackets.empty()) {
        const std::scoped_lock lock(_freeUserSendPackets._mutex);
        _freeUserSendPackets.splice(std::move(freePackets));
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

void Connection::AddAcks(const SelectiveAckBuffer::Acks& acks)
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
    {
        const std::scoped_lock lock(_freeRecvPackets._mutex);
        freePackets.splice(std::move(_freeRecvPackets));
    }

    return freePackets;
}

IPacket::List Connection::GetFreeSendPackets()
{
    IPacket::List freePackets;
    {
        IPacket::List const freePackets = _inFlightQueue->GetAllPackets();
    }

    return freePackets;
}

void Connection::AddFreeUserSendPackets(IPacket::List&& freePackets)
{
    if (!freePackets.empty()) {
        const std::scoped_lock lock(_freeUserSendPackets._mutex);
        _freeUserSendPackets.splice(std::move(freePackets));

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
