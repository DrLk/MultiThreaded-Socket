#include "Connection.h"

#include "BufferOwner.h"
#include "IConnectionState.h"

namespace FastTransport
{
    namespace Protocol
    {

        void Connection::OnRecvPackets(std::shared_ptr<BufferOwner>& packet)
        {
            _state = _state->OnRecvPackets(packet, *this);
        }

        void Connection::Send(std::vector<char>&& data)
        {
            _sendUserData.push_back(std::move(data));
        }

        std::vector<char> Connection::Recv(int size)
        {
            return std::vector<char>();
        }

        const ConnectionKey& Connection::GetConnectionKey() const
        {
            return _key;
        }

        std::list<OutgoingPacket>& Connection::GetPacketsToSend()
        {
            return _sendQueue.GetPacketsToSend();
        }

        void Connection::SendPacket(std::shared_ptr<BufferOwner>& packet, bool needAck /*= true*/)
        {
            _sendQueue.SendPacket(packet, needAck);
        }

        void Connection::Close()
        {
            throw std::runtime_error("Not implemented");
        }

        void Connection::Run()
        {
            _state->SendPackets(*this);
        }
    }
}
