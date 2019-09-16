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

        void Connection::Send(const std::vector<char>& data)
        {
            throw std::runtime_error("Not implemented");
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
            return _sendQueue.GetPacketToSend();
        }

        void Connection::SendPacket(std::shared_ptr<BufferOwner>& packet)
        {
            _sendQueue.SendPacket(packet);
        }

        void Connection::Close()
        {
            throw std::runtime_error("Not implemented");
        }

        void Connection::Run()
        {

            _state->SendPackets(*this);
            /*std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
            BufferOwner::ElementType& element = _freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(_freeBuffers, std::move(element));
            _freeBuffers.pop_back();

            synPacket->GetHeader().SetMagic();
            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(GetConnectionKey()._id);
            synPacket->GetHeader().SetSeqNumber(GetCurrentSeqNumber());*/

            auto acks = _recvQueue.GetSelectiveAcks();
        }
    }
}
