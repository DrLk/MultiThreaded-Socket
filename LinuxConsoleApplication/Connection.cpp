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

        void Connection::ProcessAcks(const SelectiveAckBuffer& acks)
        {
            _sendQueue->ProcessAcks(acks);
        }

        void Connection::ProcessPackets(std::shared_ptr<BufferOwner>& packet)
        {
            throw std::runtime_error("Not implemented");
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
        
        SeqNumberType Connection::GetCurrentSeqNumber()
        {
            return _seqNumber++;
        }

        std::list<BufferOwner::Ptr>&& Connection::GetPacketsToSend()
        {
                std::lock_guard<std::mutex> lock(_packetsToSend._mutex);
                return std::move(_packetsToSend);
        }

        void Connection::SendPacket(std::shared_ptr<BufferOwner>& packet)
        {
            std::lock_guard<std::mutex> lock(_packetsToSend._mutex);
            _packetsToSend.push_back(packet);
        }

        void Connection::Connect()
        {
            std::lock_guard<std::mutex> lock(_freeBuffers._mutex);
            BufferOwner::ElementType& element = _freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(_freeBuffers, std::move(element));
            _freeBuffers.pop_back();

            synPacket->GetHeader().SetMagic();
            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(GetConnectionKey()._id);
            synPacket->GetHeader().SetSeqNumber(GetCurrentSeqNumber());
            SendPacket(synPacket);
        }


        void Connection::Close()
        {
            throw std::runtime_error("Not implemented");
        }
    }
}
