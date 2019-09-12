#include "IConnectionState.h"


#include "BufferOwner.h"
#include "Connection.h"

namespace FastTransport
{
    namespace Protocol
    {

        Connection* ListenState::Listen(std::shared_ptr<BufferOwner>& packet, ConnectionID myID)
        {
            if (packet->GetHeader().GetPacketType() == PacketType::SYN)
            {
                return new Connection(packet->GetAddr(), myID, packet->GetHeader().GetConnectionID());
            }

            return nullptr;
        }

        IConnectionState* WaitingSynAck::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            auto header = packet->GetHeader();
            if (!header.IsValid())
            {
                return this;
                socket.Close();
            }


            return this;
        }
    }
}
