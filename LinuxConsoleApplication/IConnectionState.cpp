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
                Connection* connection =  new Connection(new WaitingSynState(),packet->GetAddr(), myID, packet->GetHeader().GetConnectionID());
                connection->OnRecvPackets(packet);
                return connection;
            }

            return nullptr;
        }

        IConnectionState* SynState::SendPackets(Connection& connection)
        {
            std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
            BufferOwner::ElementType& element = connection._freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetMagic();
            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(connection.GetConnectionKey()._id);
            synPacket->GetHeader().SetSeqNumber(connection.GetNextSeqNumber());

            connection.SendPacket(synPacket);

            connection._state = new WaitingSynAckState();
            return connection._state;
        }

        IConnectionState* WaitingSynState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& connection)
        {
            auto header = packet->GetHeader();
            if (!header.IsValid())
            {
                connection.Close();
                return this;
            }

            connection._recvQueue->AddPacket(packet);
            connection._destinationID = packet->GetHeader().GetConnectionID();


            connection._state = new SendingSynAckState();
            return connection._state;
        }

        IConnectionState* SendingSynAckState::SendPackets(Connection& connection)
        {
            std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
            BufferOwner::ElementType& element = connection._freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
            connection._freeBuffers.pop_back();

            synPacket->GetSynAckHeader().SetMagic();
            synPacket->GetSynAckHeader().SetPacketType(PacketType::SYN_ACK);
            synPacket->GetSynAckHeader().SetConnectionID(connection._destinationID);
            synPacket->GetSynAckHeader().SetSeqNumber(connection.GetNextSeqNumber());
            synPacket->GetSynAckHeader().SetRemoteConnectionID(connection.GetConnectionKey()._id);

            connection.SendPacket(synPacket);

            connection._state = new DataState();
            return connection._state;
        }

        IConnectionState* WaitingSynAckState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& connection)
        {
            auto synAckHeader = packet->GetSynAckHeader();

            if (!synAckHeader.IsValid())
            {
                connection.Close();
                return this;
            }

            connection._destinationID = synAckHeader.GetRemoteConnectionID();

            connection._state = new DataState();
            return connection._state;
        }

        IConnectionState* DataState::SendPackets(Connection& connection)
        {
            return this;
        }

        IConnectionState* DataState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& connection)
        {
            return this;
        }

    }
}
