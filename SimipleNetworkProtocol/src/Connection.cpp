#include "Connection.hpp"

#include "IConnectionState.hpp"

namespace FastTransport::Protocol {

IPacket::PairList Connection::OnRecvPackets(IPacket::Ptr&& packet)
{
    _lastReceivedPacket = Connection::DefaultTimeOut;
    auto freePackets = _state->OnRecvPackets(std::move(packet), *this);

    {
        std::lock_guard lock(_freeRecvPackets._mutex);
        freePackets.first.splice(std::move(_freeRecvPackets));
    }

    return freePackets;
}

void Connection::Send(IPacket::Ptr&& data)
{
    _sendUserData.push_back(std::move(data));
}

IPacket::List Connection::Send(IPacket::List&& data)
{
    std::lock_guard lock(_sendUserDataMutex);
    _sendUserData.splice(std::move(data));

    throw std::runtime_error("Not Implemented");
}

IPacket::List Connection::Recv(IPacket::List&& freePackets)
{
    {
        std::lock_guard lock(_freeRecvPackets._mutex);
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
    return _sendQueue.GetPacketsToSend();
}

void Connection::ProcessSentPackets(OutgoingPacket::List&& packets)
{
    auto freePackets = _inFlightQueue.AddQueue(std::move(packets));

    {
        std::lock_guard lock(_freeSendPackets._mutex);
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

void Connection::Close()
{
    throw std::runtime_error("Not implemented");
}

void Connection::Run()
{
    if (_lastReceivedPacket.count() == 0) {
        _state->OnTimeOut(*this);
    } else {
        _state->SendPackets(*this);
    }
}
} // namespace FastTransport::Protocol
