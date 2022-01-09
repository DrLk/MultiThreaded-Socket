#include "IConnectionState.hpp"

#include "Connection.hpp"
#include "IPacket.hpp"

namespace FastTransport::Protocol {

std::pair<Connection*, IPacket::List> ListenState::Listen(IPacket::Ptr&& packet, ConnectionID myID)
{
    if (packet->GetHeader().GetPacketType() == PacketType::SYN) {
        auto* connection = new Connection(new WaitingSynState(), packet->GetDstAddr(), myID); // NOLINT
        auto freePackets = connection->OnRecvPackets(std::move(packet));
        return { connection, std::move(freePackets.first) };
    }

    return { nullptr, IPacket::List() };
}

IConnectionState* SendingSynState::SendPackets(Connection& connection)
{
    std::lock_guard lock(connection._freeSendPackets._mutex);
    IPacket::Ptr synPacket = std::move(connection._freeSendPackets.back());
    connection._freeSendPackets.pop_back();

    synPacket->GetHeader().SetPacketType(PacketType::SYN);
    synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

    connection.SendPacket(std::move(synPacket), false);

    return new WaitingSynAckState();
}

IPacket::PairList WaitingSynState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    IPacket::PairList freePackets;
    auto header = packet->GetHeader();
    if (!header.IsValid()) {
        connection.Close();
        freePackets.first.push_back(std::move(packet));
        return freePackets;
    }

    if (header.GetPacketType() == PacketType::SYN) {
        connection._destinationID = packet->GetHeader().GetSrcConnectionID();
        connection._state = new SendingSynAckState();

        freePackets.first.push_back(std::move(packet));
        return freePackets;
    }
    throw std::runtime_error("Wrong packet type");
}

IConnectionState* SendingSynAckState::SendPackets(Connection& connection)
{
    std::lock_guard lock(connection._freeSendPackets._mutex);
    IPacket::Ptr synPacket = std::move(connection._freeSendPackets.back());
    connection._freeSendPackets.pop_back();

    synPacket->GetHeader().SetPacketType(PacketType::SYN_ACK);
    synPacket->GetHeader().SetDstConnectionID(connection._destinationID);
    synPacket->GetHeader().SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

    connection.SendPacket(std::move(synPacket), false);

    return new DataState();
}

IPacket::PairList WaitingSynAckState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    IPacket::PairList freePackets;
    auto header = packet->GetHeader();

    if (!header.IsValid()) {
        connection.Close();
        freePackets.first.push_back(std::move(packet));
        return freePackets;
    }

    switch (header.GetPacketType()) {
    case PacketType::SYN_ACK: {
        auto synAckHeader = packet->GetHeader();
        connection._destinationID = synAckHeader.GetSrcConnectionID();
        connection._state = new DataState();
        freePackets.first.push_back(std::move(packet));
        break;
    }
    case PacketType::DATA: {
        auto freeRecvPacket = connection.GetRecvQueue().AddPacket(std::move(packet));
        if (freeRecvPacket) {
            freePackets.first.push_back(std::move(freeRecvPacket));
        }
        break;
    }
    default: {
        throw std::runtime_error("Wrong packet type");
    }
    }

    return freePackets;
}

IConnectionState* WaitingSynAckState::OnTimeOut(Connection& /*connection*/)
{
    return new SendingSynState();
}

IConnectionState* DataState::SendPackets(Connection& connection)
{
    std::list<SeqNumberType> acks = connection.GetRecvQueue().GetSelectiveAcks();
    // TODO: maybe error after std::move second loop
    while (!acks.empty()) {

        IPacket::Ptr packet;
        {
            std::lock_guard lock(connection._freeSendPackets._mutex);
            packet = std::move(connection._freeSendPackets.back());
            connection._freeSendPackets.pop_back();
        }

        packet->GetHeader().SetPacketType(PacketType::SACK);
        packet->GetHeader().SetSrcConnectionID(connection._key.GetID());
        packet->GetHeader().SetDstConnectionID(connection._destinationID);
        packet->GetHeader().SetAckNumber(connection.GetRecvQueue().GetLastAck());
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        std::list<SeqNumberType> packetAcks;
        auto end = acks.begin();

        if (acks.size() > SelectiveAckBuffer::Acks::MaxAcks) {
            for (unsigned int i = 0; i < SelectiveAckBuffer::Acks::MaxAcks; i++) {
                end++;
            }
            packetAcks.splice(packetAcks.end(), acks, acks.begin(), end);
        } else {
            packetAcks = std::move(acks);
            // TODO: Is it needed to call clear???
            acks.clear();
        }

        packet->GetAcks().SetAcks(packetAcks);

        connection.SendPacket(std::move(packet), false);
    }

    {
        OutgoingPacket::List packets = connection.GetInFlightQueue().CheckTimeouts();
        if (!packets.empty()) {
            connection.GetSendQueue().ReSendPackets(std::move(packets));
        }
    }

    IPacket::List userData;
    {
        std::lock_guard lock(connection._sendUserDataMutex);
        userData = std::move(connection._sendUserData);
    }

    for (auto& packet : userData) {
        packet->GetHeader().SetPacketType(PacketType::DATA);
        packet->GetHeader().SetSrcConnectionID(connection.GetConnectionKey().GetID());
        packet->GetHeader().SetDstConnectionID(connection._destinationID);
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        // packet->GetPayload().SetPayload(data);
        connection.SendPacket(std::move(packet), true);
    }

    return this;
}

IPacket::PairList DataState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    IPacket::PairList freePackets;
    auto header = packet->GetHeader();

    switch (header.GetPacketType()) {
    case PacketType::SYN_ACK: {
        freePackets.first.push_back(std::move(packet));
        break;
    }
    case PacketType::SACK: {
        auto freeFilghtPackets = connection.GetInFlightQueue().ProcessAcks(packet->GetAcks());
        // send free;
        freePackets.first.push_back(std::move(packet));
        freePackets.second.splice(std::move(freeFilghtPackets));

        break;
    }
    case PacketType::DATA: {
        auto freePacket = connection.GetRecvQueue().AddPacket(std::move(packet));
        if (freePacket) {
            freePackets.first.push_back(std::move(freePacket));
        }
        break;
    }
    default: {
        throw std::runtime_error("Wrong packety type");
    }
    }
    return freePackets;
}

IConnectionState* DataState::OnTimeOut(Connection& connection)
{
    connection.Close();
    return this;
}

} // namespace FastTransport::Protocol
