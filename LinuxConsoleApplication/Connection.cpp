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
            return std::move(_packetsToSend);
        }
    }
}
