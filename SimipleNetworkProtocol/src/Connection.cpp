#include "Connection.hpp"

#include "IConnectionState.hpp"

namespace FastTransport::Protocol {

IPacket::List Connection::OnRecvPackets(IPacket::Ptr&& packet)
{
    _lastReceivedPacket = Connection::DefaultTimeOut;
    auto freePackets = _state->OnRecvPackets(std::move(packet), *this);

    {
        const std::lock_guard lock(_freeRecvPackets._mutex);
        freePackets.splice(std::move(_freeRecvPackets));
    }

    return freePackets;
}

void Connection::Send(IPacket::Ptr&& data)
{
    _sendUserData.push_back(std::move(data));
}

IPacket::List Connection::Send(IPacket::List&& data)
{
    const std::lock_guard lock(_sendUserDataMutex);
    _sendUserData.splice(std::move(data));

    throw std::runtime_error("Not Implemented");
}

IPacket::List Connection::Recv(IPacket::List&& freePackets)
{
    {
        const std::lock_guard lock(_freeRecvPackets._mutex);
        _freeRecvPackets.splice(std::move(freePackets));
    }

    return _recvQueue.GetUserData();
}

const ConnectionKey& Connection::GetConnectionKey() const
{
    return _key;
}

OutgoingPacket::List Connection::GetPacketsToSend()
{
    const std::size_t size = _inFlightQueue.GetNumberPacketToSend();
    return _sendQueue.GetPacketsToSend(size);
}

void Connection::ProcessSentPackets(OutgoingPacket::List&& packets)
{
    auto freePackets = _inFlightQueue.AddQueue(std::move(packets));

    {
        const std::lock_guard lock(_freeSendPackets._mutex);
        _freeSendPackets.splice(std::move(freePackets));
    }
}

void Connection::ProcessRecvPackets()
{
    _recvQueue.ProccessUnorderedPackets();
}

void Connection::SendPacket(IPacket::Ptr&& packet, bool needAck)
{
    _sendQueue.SendPacket(std::move(packet), needAck);
}

IPacket::List Connection::GetFreeRecvPackets()
{
    IPacket::List freePackets;
    {
        const std::lock_guard lock(_freeRecvPackets._mutex);
        freePackets.splice(std::move(_freeRecvPackets));
    }

    return freePackets;
}

void Connection::Close()
{
    throw std::runtime_error("Not implemented");
}

void Connection::Run()
{
    if (_lastReceivedPacket.count() == 0) {
        _state->OnTimeOut(*this);
    } else {
        _state = _state->SendPackets(*this);
        _state->ProcessInflightPackets(*this);
    }
}
} // namespace FastTransport::Protocol
