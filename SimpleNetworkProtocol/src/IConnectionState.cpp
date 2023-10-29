#include "IConnectionState.hpp"

#include <list>
#include <stdexcept>

#include "ConnectionKey.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "LockedList.hpp"
#include "Logger.hpp"
#include "OutgoingPacket.hpp"

#define TRACER() LOGGER() << "[" << connection.GetConnectionKey() << "-" << connection._destinationID << "]: " // NOLINT(cppcoreguidelines-macro-usage)

namespace FastTransport::Protocol {

using namespace std::chrono_literals;

std::pair<Connection::Ptr, IPacket::List> ListenState::Listen(IPacket::Ptr&& packet, ConnectionID myID)
{
    if (packet->GetPacketType() == PacketType::SYN) {
        Connection::Ptr connection = std::make_shared<Connection>(new WaitingSynState(), packet->GetDstAddr(), myID); // NOLINT
        auto freePackets = connection->OnRecvPackets(std::move(packet));
        return { connection, std::move(freePackets) };
    }

    return { nullptr, IPacket::List() };
}

IPacket::List BasicConnectionState::OnRecvPackets(IPacket::Ptr&& /*packet*/, Connection& /*connection*/)
{
    throw std::runtime_error("Not implemented");
}

IConnectionState* BasicConnectionState::SendPackets(Connection& /*connection*/)
{
    return this;
}

IConnectionState* BasicConnectionState::OnTimeOut(Connection& /*connection*/)
{
    return this;
}

std::chrono::milliseconds BasicConnectionState::GetTimeout() const
{
    return 3s;
}

void BasicConnectionState::ProcessInflightPackets(Connection& connection)
{
    IPacket::List freePackets = connection.ProcessAcks();
    connection.AddFreeUserSendPackets(std::move(freePackets));
}

IConnectionState* SendingSynState::SendPackets(Connection& connection)
{
    TRACER() << "SendingSynState::SendPackets";

    IPacket::Ptr synPacket = std::move(connection._freeInternalSendPackets.back());
    connection._freeInternalSendPackets.pop_back();

    synPacket->SetPacketType(PacketType::SYN);
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

    connection._state = new WaitingSynAckState();
    connection.SendPacket(std::move(synPacket), false);

    return new WaitingSynAckState();
}

IPacket::List WaitingSynState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    TRACER() << "WaitingSynState::OnRecvPackets";

    IPacket::List freePackets;
    if (!packet->IsValid()) {
        connection.Close();
        freePackets.push_back(std::move(packet));
        return freePackets;
    }

    if (packet->GetPacketType() == PacketType::SYN) {
        connection._destinationID = packet->GetSrcConnectionID();
        connection._state = new SendingSynAckState();

        freePackets.push_back(std::move(packet));
        return freePackets;
    }
    throw std::runtime_error("Wrong packet type");
}

IConnectionState* SendingSynAckState::SendPackets(Connection& connection)
{
    TRACER() << "SendingSynAckState::SendPackets";

    IPacket::Ptr synPacket = std::move(connection._freeInternalSendPackets.back());
    connection._freeInternalSendPackets.pop_back();

    synPacket->SetPacketType(PacketType::SYN_ACK);
    synPacket->SetDstConnectionID(connection._destinationID);
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

    connection._state = new DataState();
    connection.SendPacket(std::move(synPacket), false);

    return new DataState();
}

IPacket::List WaitingSynAckState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    TRACER() << "WaitingSynAckState::OnRecvPackets";

    IPacket::List freePackets;

    if (!packet->IsValid()) {
        connection.Close();
        freePackets.push_back(std::move(packet));
        return freePackets;
    }

    switch (packet->GetPacketType()) {
    case PacketType::SYN_ACK: {
        connection._destinationID = packet->GetSrcConnectionID();
        connection.SetConnected(true);
        connection._state = new DataState();
        freePackets.push_back(std::move(packet));
        break;
    }
    case PacketType::DATA: {
        auto freeRecvPacket = connection.GetRecvQueue().AddPacket(std::move(packet));
        if (freeRecvPacket) {
            freePackets.push_back(std::move(freeRecvPacket));
        }
        break;
    }
    default: {
        throw std::runtime_error("Wrong packet type");
    }
    }

    return freePackets;
}

IConnectionState* WaitingSynAckState::OnTimeOut(Connection& connection)
{
    TRACER() << "WaitingSynAckState::OnTimeOut";

    return new SendingSynState();
}

IConnectionState* DataState::SendPackets(Connection& connection)
{
    std::list<SeqNumberType> acks = connection.GetRecvQueue().GetSelectiveAcks();
    // TODO: maybe error after std::move second loop
    while (!acks.empty()) {
        IPacket::Ptr packet = std::move(connection._freeInternalSendPackets.back());
        connection._freeInternalSendPackets.pop_back();

        packet->SetPacketType(PacketType::SACK);
        packet->SetSrcConnectionID(connection._key.GetID());
        packet->SetDstConnectionID(connection._destinationID);
        packet->SetAckNumber(connection.GetRecvQueue().GetLastAck());
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        std::vector<SeqNumberType> packetAcks;
        packetAcks.reserve(MaxAcksSize);

        for (auto ack = acks.begin(); ack != acks.end();) {
            if (packetAcks.size() > MaxAcksSize) {
                break;
            }

            packetAcks.push_back(*ack);
            acks.erase(ack++);
        }

        packet->SetAcks(packetAcks);

        connection.SendPacket(std::move(packet), false);
    }

    {
        OutgoingPacket::List packets = connection.CheckTimeouts();
        if (!packets.empty()) {
            connection.ReSendPackets(std::move(packets));
        }
    }

    IPacket::List userData;
    connection._sendUserData.LockedSwap(userData);

    for (auto& packet : userData) {
        packet->SetPacketType(PacketType::DATA);
        packet->SetSrcConnectionID(connection.GetConnectionKey().GetID());
        packet->SetDstConnectionID(connection._destinationID);
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        // packet->GetPayload().SetPayload(data);
        connection.SendPacket(std::move(packet), true);
    }

    return this;
}

IPacket::List DataState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    IPacket::List freePackets;

    switch (packet->GetPacketType()) {
    case PacketType::SYN_ACK: {
        freePackets.push_back(std::move(packet));
        break;
    }
    case PacketType::SACK: {
        connection.AddAcks(packet->GetAcks());
        // send free;
        freePackets.push_back(std::move(packet));

        break;
    }
    case PacketType::DATA: {
        auto freePacket = connection.GetRecvQueue().AddPacket(std::move(packet));
        if (freePacket) {
            freePackets.push_back(std::move(freePacket));
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
    TRACER() << "DataState::OnTimeOut";

    connection.Close();
    return this;
}

std::chrono::milliseconds DataState::GetTimeout() const
{
    return 30s;
}

} // namespace FastTransport::Protocol
