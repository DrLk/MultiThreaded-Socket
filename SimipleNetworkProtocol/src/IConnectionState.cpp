#include "IConnectionState.h"

#include "Connection.h"
#include "IPacket.h"

namespace FastTransport
{
    namespace Protocol
    {

        Connection* ListenState::Listen(std::unique_ptr<IPacket>&& packet, ConnectionID myID)
        {
            if (packet->GetHeader().GetPacketType() == PacketType::SYN)
            {
                Connection* connection =  new Connection(new WaitingSynState(),packet->GetDstAddr(), myID);
                connection->OnRecvPackets(std::move(packet));
                return connection;
            }

            return nullptr;
        }

        IConnectionState* SendingSynState::SendPackets(Connection& connection)
        {
            std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
            std::unique_ptr<IPacket> synPacket = std::move(connection._freeBuffers.back());
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);

            connection.SendPacket(std::move(synPacket), false);

            connection._state = new WaitingSynAckState();
            return connection._state;
        }

        IConnectionState* WaitingSynState::OnRecvPackets(std::unique_ptr<IPacket>&& packet, Connection& connection)
        {
            auto header = packet->GetHeader();
            if (!header.IsValid())
            {
                connection.Close();
                return this;
            }

            if (header.GetPacketType() == PacketType::SYN)
            {
                connection._destinationID = packet->GetHeader().GetSrcConnectionID();
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
            std::unique_ptr<IPacket> synPacket = std::move(connection._freeBuffers.back());
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synPacket->GetHeader().SetDstConnectionID(connection._destinationID);
            synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);

            connection.SendPacket(std::move(synPacket), false);

            connection._state = new DataState();
            return connection._state;
        }

        IConnectionState* WaitingSynAckState::OnRecvPackets(std::unique_ptr<IPacket>&& packet, Connection& connection)
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
                    auto synAckHeader = packet->GetHeader();
                    connection._destinationID = synAckHeader.GetSrcConnectionID();
                    connection._state = new DataState();
                    break;
                }
            case PacketType::DATA:
                {
                    connection._recvQueue.AddPacket(std::move(packet));
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
            //TODO: maybe error after std::move second loop
            while (!acks.empty())
            {
                std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
                std::unique_ptr<IPacket> packet = std::move(connection._freeBuffers.back());
                connection._freeBuffers.pop_back();

                packet->GetHeader().SetPacketType(PacketType::SACK);
                packet->GetHeader().SetSrcConnectionID(connection._key._id);
                packet->GetHeader().SetDstConnectionID(connection._destinationID);
                packet->GetHeader().SetAckNumber(connection._recvQueue.GetLastAck());


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
                    //TODO: Is it needed to call clear???
                    acks.clear();
                }

                packet->GetAcks().SetAcks(packetAcks);

                connection.SendPacket(std::move(packet), false);
            }

            {
                std::list<OutgoingPacket> packets = connection._inFlightQueue.CheckTimeouts();
                connection._sendQueue.ReSendPackets(std::move(packets));
            }

            std::list<std::unique_ptr<IPacket>> userData;
            {
                std::lock_guard<std::mutex> lock(connection._sendUserDataMutex);
                userData = std::move(connection._sendUserData);
            }

            for (auto& packet : userData)
            {
                packet->GetHeader().SetPacketType(PacketType::DATA);
                packet->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);
                packet->GetHeader().SetDstConnectionID(connection._destinationID);

                //packet->GetPayload().SetPayload(data);
                connection.SendPacket(std::move(packet));
            }

            return this;
        }

        IConnectionState* DataState::OnRecvPackets(std::unique_ptr<IPacket>&& packet, Connection& connection)
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
                    auto freePackets = connection._inFlightQueue.ProcessAcks(packet->GetAcks());
                    {
                        std::lock_guard<std::mutex> lock(connection._freeBuffers._mutex);
                        connection._freeBuffers.splice(connection._freeBuffers.end(), std::move(freePackets));
                    }
                    break;
                }
            case PacketType::DATA:
                {
                    connection._recvQueue.AddPacket(std::move(packet));
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
