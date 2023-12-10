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
#include "IPacket.hpp"
#include "IRecvQueue.hpp"
#include "Logger.hpp"
#include "OutgoingPacket.hpp"

#define TRACER() LOGGER() << "[" << connection.GetConnectionKey() << "-" << connection._destinationID << "]: " // NOLINT(cppcoreguidelines-macro-usage)

using namespace std::literals::chrono_literals;

namespace FastTransport::Protocol {

std::pair<Connection::Ptr, IPacket::List> ListenState::Listen(IPacket::Ptr&& packet, ConnectionID myID)
{
    if (packet->GetPacketType() == PacketType::SYN) {
        Connection::Ptr connection = std::make_shared<Connection>(ConnectionState::WaitingSynState, packet->GetDstAddr(), myID); // NOLINT
        auto freePackets = connection->OnRecvPackets(std::move(packet));
        return { connection, std::move(freePackets) };
    }

    return { nullptr, IPacket::List() };
}

std::tuple<ConnectionState, IPacket::List> BasicConnectionState::OnRecvPackets(IPacket::Ptr&& /*packet*/, Connection& /*connection*/) // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    throw std::runtime_error("Not implemented");
}

ConnectionState BasicConnectionState::SendPackets(Connection& /*connection*/)
{
    throw std::runtime_error("Not implemented");
}

ConnectionState BasicConnectionState::OnTimeOut(Connection& /*connection*/)
{
    throw std::runtime_error("Not implemented");
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

ConnectionState SendingSynState::SendPackets(Connection& connection)
{
    TRACER() << "SendingSynState::SendPackets";

    IPacket::Ptr synPacket = std::move(connection._freeInternalSendPackets.back());
    connection._freeInternalSendPackets.pop_back();

    synPacket->SetPacketType(PacketType::SYN);
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());
    synPacket->SetPayload(std::span<IPacket::ElementType>());

    connection.SendPacket(std::move(synPacket), false);

    return ConnectionState::WaitingSynAckState;
}

std::tuple<ConnectionState, IPacket::List> WaitingSynState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
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

    if (packet->GetPacketType() == PacketType::SYN) {
        connection._destinationID = packet->GetSrcConnectionID();

        freePackets.push_back(std::move(packet));
        connectionState = ConnectionState::SendingSynAckState;
        return result;
    }
    throw std::runtime_error("Wrong packet type");
}

ConnectionState SendingSynAckState::SendPackets(Connection& connection)
{
    TRACER() << "SendingSynAckState::SendPackets";

    IPacket::Ptr synPacket = std::move(connection._freeInternalSendPackets.back());
    connection._freeInternalSendPackets.pop_back();

    synPacket->SetPacketType(PacketType::SYN_ACK);
    synPacket->SetDstConnectionID(connection._destinationID);
    synPacket->SetSrcConnectionID(connection.GetConnectionKey().GetID());
    synPacket->SetAddr(connection.GetConnectionKey().GetDestinaionAddr());
    synPacket->SetPayload(std::span<IPacket::ElementType>());

    connection.SendPacket(std::move(synPacket), false);

    return ConnectionState::DataState;
}

std::tuple<ConnectionState, IPacket::List> WaitingSynAckState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
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
    case PacketType::SYN_ACK: {
        connection._destinationID = packet->GetSrcConnectionID();
        connection.SetConnected(true);
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

    connectionState = ConnectionState::DataState;
    return result;
}

ConnectionState WaitingSynAckState::SendPackets(Connection& /*connection*/)
{
    return ConnectionState::WaitingSynAckState;
}

ConnectionState WaitingSynAckState::OnTimeOut(Connection& connection)
{
    TRACER() << "WaitingSynAckState::OnTimeOut";

    return ConnectionState::SendingSynState;
}

ConnectionState DataState::SendPackets(Connection& connection)
{
    std::list<SeqNumberType> acks = connection.GetRecvQueue().GetSelectiveAcks();
    // TODO: maybe error after std::move second loop
    while (!acks.empty()) {
        IPacket::Ptr packet = std::move(connection._freeInternalSendPackets.back());
        connection._freeInternalSendPackets.pop_back();

        packet->SetPacketType(PacketType::SACK);
        packet->SetSrcConnectionID(connection.GetConnectionKey().GetID());
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

    return ConnectionState::DataState;
}

std::tuple<ConnectionState, IPacket::List> DataState::OnRecvPackets(IPacket::Ptr&& packet, Connection& connection)
{
    std::tuple<ConnectionState, IPacket::List> result;
    auto& [connectionState, freePackets] = result;

    switch (packet->GetPacketType()) {
    case PacketType::SYN_ACK: {
        freePackets.push_back(std::move(packet));
        break;
    }
    case PacketType::SACK: {
        connection.SetLastAck(packet->GetAckNumber());
        connection.AddAcks(packet->GetAcks());
        // send free;
        freePackets.push_back(std::move(packet));

        break;
    }
    case PacketType::DATA: {
        connection.SetLastAck(packet->GetAckNumber());
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
    connectionState = ConnectionState::DataState;
    return result;
}

ConnectionState DataState::OnTimeOut(Connection& connection)
{
    TRACER() << "DataState::OnTimeOut";

    connection.Close();
    return ConnectionState::DataState;
}

std::chrono::milliseconds DataState::GetTimeout() const
{
    return 30s;
}

} // namespace FastTransport::Protocol
