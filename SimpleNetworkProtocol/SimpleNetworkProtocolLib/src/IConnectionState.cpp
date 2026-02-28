#include "IConnectionState.hpp"

#include <chrono>
#include <list>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "Connection.hpp"
#include "ConnectionKey.hpp"
#include "ConnectionState.hpp"
#include "HeaderTypes.hpp"
#include "IConnectionInternal.hpp"
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "Logger.hpp"
#include "OutgoingPacket.hpp"

#define TRACER() LOGGER() << "[" << connection.GetConnectionKey() << "-" << connection.GetDestinationID() << "]: " // NOLINT(cppcoreguidelines-macro-usage)

using namespace std::literals::chrono_literals;

namespace FastTransport::Protocol {

std::pair<Connection::Ptr, IPacket::List> ListenState::Listen(IPacket::Ptr&& packet, ConnectionID myID)
{
    if (packet->GetPacketType() == PacketType::Syn) {
        Connection::Ptr connection = std::make_shared<Connection>(ConnectionState::WaitingSynState, packet->GetDstAddr(), myID); // NOLINT
        auto freePackets = connection->OnRecvPackets(std::move(packet));
        return { connection, std::move(freePackets) };
    }

    return { nullptr, IPacket::List() };
}

std::tuple<ConnectionState, IPacket::List> BasicConnectionState::OnRecvPackets(IPacket::Ptr&& /*packet*/, IConnectionInternal& /*connection*/) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    throw std::runtime_error("Not implemented");
}

ConnectionState BasicConnectionState::SendPackets(IConnectionInternal& /*connection*/)
{
    throw std::runtime_error("Not implemented");
}

ConnectionState BasicConnectionState::OnTimeOut(IConnectionInternal& /*connection*/)
{
    throw std::runtime_error("Not implemented");
}

std::chrono::milliseconds BasicConnectionState::GetTimeout() const
{
    return 30s;
}

void BasicConnectionState::ProcessInflightPackets(IConnectionInternal& connection)
{
    IPacket::List freePackets = connection.ProcessAcks();
    connection.AddFreeUserSendPackets(std::move(freePackets));
}

ConnectionState SendingSynState::SendPackets(IConnectionInternal& connection)
{
    TRACER() << "SendingSynState::SendPackets";

    IPacket::Ptr synPacket = connection.GetFreeSendPacket();
    if (!synPacket) {
        return ConnectionState::SendingSynState;
    }

    synPacket->SetPacketType(PacketType::Syn);
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());
    synPacket->SetPayload(std::span<IPacket::ElementType>());

    IPacket::List packets;
    packets.push_back(std::move(synPacket));
    connection.SendServicePackets(std::move(packets));

    return ConnectionState::WaitingSynAckState;
}

std::tuple<ConnectionState, IPacket::List> WaitingSynState::OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection)
{
    TRACER() << "WaitingSynState::OnRecvPackets";

    std::tuple<ConnectionState, IPacket::List> result;
    auto& [connectionState, freePackets] = result;

    if (!packet->IsValid()) {
        connection.Close();
        freePackets.push_back(std::move(packet));
        connectionState = ConnectionState::WaitingSynState;
        return result;
    }

    if (packet->GetPacketType() == PacketType::Syn) {
        connection.SetDestinationID(packet->GetSrcConnectionID());

        freePackets.push_back(std::move(packet));
        connectionState = ConnectionState::SendingSynAckState;
        return result;
    }
    throw std::runtime_error("Wrong packet type");
}

ConnectionState SendingSynAckState::SendPackets(IConnectionInternal& connection)
{
    TRACER() << "SendingSynAckState::SendPackets";

    IPacket::Ptr synPacket = connection.GetFreeSendPacket();
    if (!synPacket) {
        return ConnectionState::SendingSynAckState;
    }

    synPacket->SetPacketType(PacketType::SynAck);
    synPacket->SetDstConnectionID(connection.GetDestinationID());
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());
    synPacket->SetPayload(std::span<IPacket::ElementType>());

    IPacket::List packets;
    packets.push_back(std::move(synPacket));
    connection.SendServicePackets(std::move(packets));

    return ConnectionState::DataState;
}

std::tuple<ConnectionState, IPacket::List> WaitingSynAckState::OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection)
{
    TRACER() << "WaitingSynAckState::OnRecvPackets";

    std::tuple<ConnectionState, IPacket::List> result;
    auto& [connectionState, freePackets] = result;

    if (!packet->IsValid()) {
        connection.Close();
        freePackets.push_back(std::move(packet));
        connectionState = ConnectionState::WaitingSynAckState;
        return result;
    }

    switch (packet->GetPacketType()) {
    case PacketType::SynAck: {
        connection.SetDestinationID(packet->GetSrcConnectionID());
        connection.SetConnected(true);
        freePackets.push_back(std::move(packet));
        break;
    }
    case PacketType::Data: {
        connection.SetDestinationID(packet->GetSrcConnectionID());
        connection.SetConnected(true);
        auto freeRecvPacket = connection.RecvPacket(std::move(packet));
        if (freeRecvPacket) {
            freePackets.push_back(std::move(freeRecvPacket));
        }
        break;
    }
    default: {
        throw std::runtime_error("Wrong packet type");
    }
    }

    connectionState = ConnectionState::DataState;
    return result;
}

ConnectionState WaitingSynAckState::SendPackets(IConnectionInternal& /*connection*/)
{
    return ConnectionState::WaitingSynAckState;
}

ConnectionState WaitingSynAckState::OnTimeOut(IConnectionInternal& connection)
{
    TRACER() << "WaitingSynAckState::OnTimeOut";

    return ConnectionState::SendingSynState;
}

ConnectionState DataState::SendPackets(IConnectionInternal& connection)
{
    std::list<SeqNumberType> acks = connection.GetRecvQueue().GetSelectiveAcks();
    auto lastAck = connection.GetRecvQueue().GetLastAck();
    // TODO: maybe error after std::move second loop
    while (!acks.empty()) {
        IPacket::Ptr packet = connection.GetFreeSendPacket();
        if (!packet) {
            break;
        }

        packet->SetPacketType(PacketType::Sack);
        packet->SetSrcConnectionID(connection.GetConnectionKey().GetID());
        packet->SetDstConnectionID(connection.GetDestinationID());
        packet->SetAckNumber(connection.GetRecvQueue().GetLastAck());
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        std::vector<SeqNumberType> packetAcks;
        packetAcks.reserve(MaxAcksSize);

        for (auto ack = acks.begin(); ack != acks.end();) {
            if (packetAcks.size() > MaxAcksSize) {
                break;
            }

            if (*ack >= lastAck) 
            {
                packetAcks.push_back(*ack);
            }

            ack = acks.erase(ack);
        }

        packet->SetAcks(packetAcks);

        IPacket::List packets;
        packets.push_back(std::move(packet));
        connection.SendServicePackets(std::move(packets));
    }

    {
        OutgoingPacket::List packets = connection.CheckTimeouts();
        if (!packets.empty()) {
            connection.ReSendPackets(std::move(packets));
        }
    }

    IPacket::List userData = connection.GetSendUserData();

    for (auto& packet : userData) {
        packet->SetPacketType(PacketType::Data);
        packet->SetSrcConnectionID(connection.GetConnectionKey().GetID());
        packet->SetDstConnectionID(connection.GetDestinationID());
        packet->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());

        IPacket::List dataPackets;
        dataPackets.push_back(std::move(packet));
        connection.SendDataPackets(std::move(dataPackets));
    }

    return ConnectionState::DataState;
}

std::tuple<ConnectionState, IPacket::List> DataState::OnRecvPackets(IPacket::Ptr&& packet, IConnectionInternal& connection)
{
    std::tuple<ConnectionState, IPacket::List> result;
    auto& [connectionState, freePackets] = result;

    switch (packet->GetPacketType()) {
    case PacketType::SynAck: {
        freePackets.push_back(std::move(packet));
        break;
    }
    case PacketType::Sack: {
        connection.SetLastAck(packet->GetAckNumber());
        connection.AddAcks(packet->GetAcks());
        // send free;
        freePackets.push_back(std::move(packet));

        break;
    }
    case PacketType::Data: {
        connection.SetLastAck(packet->GetAckNumber());

        auto freePacket = connection.RecvPacket(std::move(packet));
        if (freePacket) {
            freePackets.push_back(std::move(freePacket));
        }
        break;
    }
    default: {
        throw std::runtime_error("Wrong packety type");
    }
    }
    connectionState = ConnectionState::DataState;
    return result;
}

ConnectionState DataState::OnTimeOut(IConnectionInternal& connection)
{
    TRACER() << "DataState::OnTimeOut";

    connection.Close();
    return ConnectionState::DataState;
}

std::chrono::milliseconds DataState::GetTimeout() const
{
    return 300s;
}

} // namespace FastTransport::Protocol
