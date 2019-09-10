#include "IConnectionState.h"


#include "BufferOwner.h"
#include "Connection.h"

namespace FastTransport
{
    namespace Protocol
    {
        IConnectionState* ConnectedState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            socket.ProcessAcks(packet->GetAcks());
            socket.ProcessPackets(packet);

            return this;
        }
    }
}
