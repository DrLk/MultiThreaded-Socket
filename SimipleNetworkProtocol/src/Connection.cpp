#include "Connection.h"

#include "IConnectionState.h"

namespace FastTransport
{
    namespace Protocol
    {

        IPacket::List Connection::OnRecvPackets(IPacket::Ptr&& packet)
        {
            _lastReceivedPacket = Connection::DefaultTimeOut;
            auto freePackets = _state->OnRecvPackets(std::move(packet), *this);

            return freePackets;
        }

        void Connection::Send(IPacket::Ptr&& data)
        {
            _sendUserData.push_back(std::move(data));
        }

        IPacket::List Connection::Send(IPacket::List&& data)
        {
            std::lock_guard lock(_sendUserDataMutex);
            _sendUserData.splice(_sendUserData.end(), std::move(data));

            throw std::runtime_error("Not Implemented");
        }

        IPacket::List Connection::Recv(IPacket::List&& freePackets)
        {
            {
                std::lock_guard lock(_freeRecvPackets._mutex);
                _freeRecvPackets.splice(_freeRecvPackets.end(), std::move(freePackets));
            }

            return _recvQueue.GetUserData();
        }

        const ConnectionKey& Connection::GetConnectionKey() const
        {
            return _key;
        }

        std::list<OutgoingPacket> Connection::GetPacketsToSend()
        {
            return _sendQueue.GetPacketsToSend();
        }

        void Connection::ProcessSentPackets(std::list<OutgoingPacket>&& packets)
        {
            auto freePackets = _inFlightQueue.AddQueue(std::move(packets));

            {
                std::lock_guard lock(_freeBuffers._mutex);
                _freeBuffers.splice(_freeBuffers.end(), std::move(freePackets));
            }
        }
        
        void Connection::ProcessRecvPackets()
        {
            _recvQueue.ProccessUnorderedPackets();
        }

        void Connection::SendPacket(IPacket::Ptr&& packet, bool needAck /*= true*/)
        {
            _sendQueue.SendPacket(std::move(packet), needAck);
        }

        void Connection::Close()
        {
            throw std::runtime_error("Not implemented");
        }

        void Connection::Run()
        {
            if (!_lastReceivedPacket.count())
                _state->OnTimeOut(*this);
            else
                _state->SendPackets(*this);
        }
    }
}
