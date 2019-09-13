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
                return new Connection(new WaitingSynState(),packet->GetAddr(), myID, packet->GetHeader().GetConnectionID());
            }

            return nullptr;
        }

        IConnectionState* SynState::SendPackets(Connection& socket)
        {
            std::lock_guard<std::mutex> lock(socket._freeBuffers._mutex);
            BufferOwner::ElementType& element = socket._freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(socket._freeBuffers, std::move(element));
            socket._freeBuffers.pop_back();

            synPacket->GetHeader().SetMagic();
            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(socket.GetConnectionKey()._id);
            synPacket->GetHeader().SetSeqNumber(socket.GetCurrentSeqNumber());

            socket.SendPacket(synPacket);

            socket._state = new WaitingSynAckState();
            return socket._state;
        }

        IConnectionState* WaitingSynState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            auto header = packet->GetHeader();
            if (!header.IsValid())
            {
                return this;
                socket.Close();
            }

            socket._recvQueue->AddPacket(packet);


            socket._state = new SendingSynAckState();
            return socket._state;
        }

        IConnectionState* SendingSynAckState::SendPackets(Connection& socket)
        {
            socket._state = new DataState();
            return socket._state;
        }

        IConnectionState* WaitingSynAckState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            socket._state = new DataState();
            return socket._state;
        }

        IConnectionState* DataState::SendPackets(Connection& socket)
        {
            return this;
        }

        IConnectionState* DataState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& socket)
        {
            return this;
        }

    }
}
