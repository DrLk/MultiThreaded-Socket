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
                Connection* connection =  new Connection(new WaitingSynState(),packet->GetAddr(), myID);
                connection->OnRecvPackets(packet);
                return connection;
            }

            return nullptr;
        }

        IConnectionState* SendingSynState::SendPackets(Connection& connection)
        {
            std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
            BufferOwner::ElementType& element = connection._freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetConnectionID(connection.GetConnectionKey()._id);

            connection.SendPacket(synPacket, false);

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

            if (header.GetPacketType() == PacketType::SYN)
            {
                connection._destinationID = packet->GetHeader().GetConnectionID();
                connection._state = new SendingSynAckState();

                return connection._state;
            }
            else
            {
                connection.Close();
                return this;
            }
        }

        IConnectionState* SendingSynAckState::SendPackets(Connection& connection)
        {
            std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
            BufferOwner::ElementType& element = connection._freeBuffers.back();
            auto synPacket = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
            connection._freeBuffers.pop_back();

            synPacket->GetSynAckHeader().SetPacketType(PacketType::SYN_ACK);
            synPacket->GetSynAckHeader().SetConnectionID(connection._destinationID);
            synPacket->GetSynAckHeader().SetRemoteConnectionID(connection.GetConnectionKey()._id);

            connection.SendPacket(synPacket, false);

            connection._state = new DataState();
            return connection._state;
        }

        IConnectionState* WaitingSynAckState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& connection)
        {
            auto header = packet->GetHeader();

            if (!header.IsValid())
            {
                connection.Close();
                return this;
            }

            switch (header.GetPacketType())
            {
            case PacketType::SYN_ACK:
                {
                    auto synAckHeader = packet->GetSynAckHeader();
                    connection._destinationID = synAckHeader.GetRemoteConnectionID();
                    connection._state = new DataState();
                    break;
                }
            case PacketType::DATA:
                {
                    connection._recvQueue.AddPacket(packet);
                    break;
                }
            default:
                {
                    connection.Close();
                    return this;
                }
            }

            return connection._state;
        }

        IConnectionState* WaitingSynAckState::OnTimeOut(Connection& connection)
        {
            connection._state = new SendingSynState();
            return connection._state;
        }

        IConnectionState* DataState::SendPackets(Connection& connection)
        {
            std::list<SeqNumberType> acks = std::move(connection._recvQueue.GetSelectiveAcks());
            while (!acks.empty())
            {
                std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
                BufferOwner::ElementType& element = connection._freeBuffers.back();
                auto packet = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
                connection._freeBuffers.pop_back();

                packet->GetHeader().SetPacketType(PacketType::SACK);
                packet->GetHeader().SetConnectionID(connection._destinationID);


                std::list<SeqNumberType> packetAcks;
                auto end = acks.begin();

                if (acks.size() > SelectiveAckBuffer::Acks::MaxAcks)
                {
                    for (unsigned int i = 0; i < SelectiveAckBuffer::Acks::MaxAcks; i++)
                    {
                        end++;
                    }
                    packetAcks.splice(packetAcks.begin(), acks, acks.begin(), end);
                }
                else
                {
                    packetAcks = std::move(acks);
                }

                packet->GetAcks().SetAcks(packetAcks);

                connection.SendPacket(packet, false);
            }

            connection._sendQueue.CheckTimeouts();



            std::list<std::vector<char>> userData;
            {
                std::lock_guard<std::mutex> lock(connection._sendUserData._mutex);
                userData = std::move(connection._sendUserData);
            }

            for (auto data : userData)
            {
                std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
                BufferOwner::ElementType& element = connection._freeBuffers.back();
                auto packet = std::make_shared<BufferOwner>(connection._freeBuffers, std::move(element));
                connection._freeBuffers.pop_back();

                packet->GetHeader().SetPacketType(PacketType::DATA);
                packet->GetHeader().SetConnectionID(connection._destinationID);

                packet->GetPayload().SetPayload(data);
                connection.SendPacket(packet);
            }

            return this;
        }

        IConnectionState* DataState::OnRecvPackets(std::shared_ptr<BufferOwner>& packet, Connection& connection)
        {
            auto header = packet->GetHeader();

            switch (header.GetPacketType())
            {
            case PacketType::SYN_ACK:
                {
                    break;
                }
            case PacketType::SACK:
                {
                    connection._sendQueue.ProcessAcks(packet->GetAcks());
                    break;
                }
            case PacketType::DATA:
                {
                    connection._recvQueue.AddPacket(packet);
                    break;
                }
            default:
                {
                    connection.Close();
                    return this;
                }
            }
            return this;
        }

        IConnectionState* DataState::OnTimeOut(Connection& connection)
        {
            connection.Close();
            return this;
        }

    }
}
