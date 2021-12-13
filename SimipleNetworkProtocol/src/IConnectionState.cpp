#include "IConnectionState.h"

#include "Connection.h"
#include "IPacket.h"

namespace FastTransport
{
    namespace Protocol
    {

        Connection* ListenState::Listen(IPacket::Ptr&& packet, ConnectionID myID)
        {
            if (packet->GetHeader().GetPacketType() == PacketType::SYN)
            {
                Connection* connection =  new Connection(new WaitingSynState(),packet->GetDstAddr(), myID);
                //TODO: return free packets
                connection->OnRecvPackets(std::move(packet));
                return connection;
            }

            return nullptr;
        }

        IConnectionState* SendingSynState::SendPackets(Connection& connection)
        {
            std::lock_guard lock(connection._freeBuffers._mutex);
            IPacket::Ptr synPacket = std::move(connection._freeBuffers.back());
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetPacketType(PacketType::SYN);
            synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);
            synPacket->SetAddr(connection.GetConnectionKey()._dstAddr);

            connection.SendPacket(std::move(synPacket), false);

            connection._state = new WaitingSynAckState();
            return connection._state;
        }

        IPacket::List WaitingSynState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
        {
            IPacket::List freePackets;
            auto header = packet->GetHeader();
            if (!header.IsValid())
            {
                connection.Close();
                return freePackets;
            }

            if (header.GetPacketType() == PacketType::SYN)
            {
                connection._destinationID = packet->GetHeader().GetSrcConnectionID();
                connection._state = new SendingSynAckState();

                return freePackets;
            }
            else
            {
                connection.Close();
                return freePackets;
            }
        }

        IConnectionState* SendingSynAckState::SendPackets(Connection& connection)
        {
            std::lock_guard lock(connection._freeBuffers._mutex);
            IPacket::Ptr synPacket = std::move(connection._freeBuffers.back());
            connection._freeBuffers.pop_back();

            synPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
            synPacket->GetHeader().SetDstConnectionID(connection._destinationID);
            synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);
            synPacket->SetAddr(connection.GetConnectionKey()._dstAddr);

            connection.SendPacket(std::move(synPacket), false);

            connection._state = new DataState();
            return connection._state;
        }

        IPacket::List WaitingSynAckState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
        {
            IPacket::List freePackets;
            auto header = packet->GetHeader();

            if (!header.IsValid())
            {
                connection.Close();
                freePackets.push_back(std::move(packet));
                return freePackets;
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
                    auto freePacket = connection._recvQueue.AddPacket(std::move(packet));
                    if (freePacket)
                        freePackets.push_back(std::move(freePacket));
                    break;
                }
            default:
                {
                    connection.Close();
                    freePackets.push_back(std::move(packet));
                    return freePackets;
                }
            }

            return freePackets;
        }

        IConnectionState* WaitingSynAckState::OnTimeOut(Connection& connection)
        {
            connection._state = new SendingSynState();
            return connection._state;
        }

        IConnectionState* DataState::SendPackets(Connection& connection)
        {
            std::list<SeqNumberType> acks = connection._recvQueue.GetSelectiveAcks();
            //TODO: maybe error after std::move second loop
            while (!acks.empty())
            {
                std::lock_guard lock(connection._freeBuffers._mutex);
                IPacket::Ptr packet = std::move(connection._freeBuffers.back());
                connection._freeBuffers.pop_back();

                packet->GetHeader().SetPacketType(PacketType::SACK);
                packet->GetHeader().SetSrcConnectionID(connection._key._id);
                packet->GetHeader().SetDstConnectionID(connection._destinationID);
                packet->GetHeader().SetAckNumber(connection._recvQueue.GetLastAck());
                packet->SetAddr(connection.GetConnectionKey()._dstAddr);


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
                if(!packets.empty())
                    connection._sendQueue.ReSendPackets(std::move(packets));
            }

            IPacket::List userData;
            {
                std::lock_guard lock(connection._sendUserDataMutex);
                userData = std::move(connection._sendUserData);
            }

            for (auto& packet : userData)
            {
                packet->GetHeader().SetPacketType(PacketType::DATA);
                packet->GetHeader().SetSrcConnectionID(connection.GetConnectionKey()._id);
                packet->GetHeader().SetDstConnectionID(connection._destinationID);
                packet->SetAddr(connection.GetConnectionKey()._dstAddr);

                //packet->GetPayload().SetPayload(data);
                connection.SendPacket(std::move(packet));
            }

            return this;
        }

        IPacket::List DataState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
        {
            IPacket::List freePackets;
            auto header = packet->GetHeader();

            switch (header.GetPacketType())
            {
            case PacketType::SYN_ACK:
                {
                    break;
                }
            case PacketType::SACK:
                {
                    freePackets = connection._inFlightQueue.ProcessAcks(packet->GetAcks());
                    break;
                }
            case PacketType::DATA:
                {
                    auto freePacket = connection._recvQueue.AddPacket(std::move(packet));
                    if (freePacket)
                        freePackets.push_back(std::move(freePacket));
                    break;
                }
            default:
                {
                    connection.Close();
                    return freePackets;
                }
            }
            return freePackets;
        }

        IConnectionState* DataState::OnTimeOut(Connection& connection)
        {
            connection.Close();
            return this;
        }

    }
}
