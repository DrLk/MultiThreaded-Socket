#include "Connection.h"

#include "IConnectionState.h"

namespace FastTransport
{
    namespace Protocol
    {

        void Connection::OnRecvPackets(std::unique_ptr<IPacket>&& packet)
        {
            _lastReceivedPacket = Connection::DefaultTimeOut;
            _state = _state->OnRecvPackets(std::move(packet), *this);
        }

        void Connection::Send(std::unique_ptr<IPacket>&& data)
        {
            _sendUserData.push_back(std::move(data));
        }

        std::list<std::unique_ptr<IPacket>> Connection::Send(std::list<std::unique_ptr<IPacket>>&& data)
        {
            std::lock_guard lock(_sendUserDataMutex);
            _sendUserData.splice(_sendUserData.end(), std::move(data));

            throw std::runtime_error("Not Implemented");
        }

        std::vector<char> Connection::Recv(int size)
        {
            throw std::runtime_error("Not Implemented");

        }

        std::list<std::unique_ptr<IPacket>> Connection::Recv(std::list<std::unique_ptr<IPacket>>&& freePackets)
        {
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

        void Connection::SendPacket(std::unique_ptr<IPacket>&& packet, bool needAck /*= true*/)
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
