#include "IConnectionState.h"


#include "BufferOwner.h"
#include "Connection.h"

namespace FastTransport
{
    namespace Protocol
    {

        Connection* ListenState::Listen(std::shared_ptr<BufferOwner>& packet)
        {
            if (packet->GetHeader().GetType() == PacketType::SYN)
                return new Connection(packet->GetAddr().GetAddr(), packet->GetHeader().GetConnectionID());

            return nullptr;
        }


        IConnectionState* ConnectedState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            socket.ProcessAcks(packet->GetAcks());
            socket.ProcessPackets(packet);

            return this;
        }
    }
}
