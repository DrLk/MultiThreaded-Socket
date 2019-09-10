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
            _sendQueue.ProcessAcks(acks);
        }

        void Connection::ProcessPackets(std::shared_ptr<BufferOwner>& packet)
        {
            _recvQueue.ProcessPackets(packet);
        }



        void Send(const std::vector<char>& data)
        {

        }


        std::vector<char> Recv(int size)
        {
            return std::vector<char>();
        }

        IConnection* Listen()
        {
            return nullptr;
        }

        IConnection* Connect()
        {
            return nullptr;
        }
    }
}
